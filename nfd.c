#include <stdlib.h>
#include <limits.h>
#include <wchar.h>
#include <stdint.h>

#include "nfd.h"
#include "decomp.h"

/* returns a composite value consisting of the canonical combining class
 * in the upper 8 bits and the codepoint value in the lower bits. */

uint32_t decomp_iterate(struct decomp_iterator *di)
{
	const unsigned char *v;
	const uint16_t *u;
	unsigned hi, mid, low;
	unsigned min, len;
	unsigned max;
	wchar_t wc;
	unsigned a, b, c;

	if (!di->cur) {
		b = *di->src;
		if (b < 128) {
			if (b) di->src++;
			return b;
		}
		int l = mbtowc(&wc, di->src, MB_LEN_MAX);
		if (l < 0) {
			wc = 0xfffd;
			l = 1;
		}
		di->src += l;

		/* Algorithmic Hangul decomposition */
		if (wc>=0xac00 && wc<=0xd7a3) {
			c = wc-0xac00;
			wc = 0x1100 + c/588;
			di->cur = di->hbuf;
			di->rem = 1;
			di->hbuf[0] = 252;
			di->hbuf[1] = (0x1161 + (c%588)/28) >> 8;
			di->hbuf[2] = (0x1161 + (c%588)/28) & 255;
			if (c%28) {
				di->hbuf[3] = 252;
				di->hbuf[4] = (0x11a7 + (c%28)) >> 8;
				di->hbuf[5] = (0x11a7 + (c%28)) & 255;
				di->rem = 2;
			}
			return wc;
		}

		if (wc>=0x30000) return wc;
		hi = wc>>12;
		a = toplevel[hi];
		if (!a) return wc;

		u = secondlevel + (a-1);
		min = u[0] & 63;
		len = (u[0] >> 6) + 1;
		mid = ((wc>>6) & 63) - min;
		u++;
		if (mid >= len) return wc;
		a = u[mid];
		if (!a) return wc;

		v = lastlevel + (a-1);
		min = v[0] & 63;
		len = (v[1] & 63) + 1;
		low = (wc & 63) - min;
		max = (v[0]>>6) + (v[1]>>6)*4;
		v += 2;
		if (low >= len) return wc;
		a = v[low];
		if (!a) return wc;

		di->cur = v + len + (a-1);
		di->rem = max;
	}
	v = di->cur;
	if (v[0] < 240) {
		c = decomp_map[v[0]];
		if (!(c & 0xffffff)) {
			c += wc;
			di->cur = 0;
			return c;
		}
		v++;
	} else if (v[0] < 252) {
		c = decomp_map[(v[0]-240)*240 + v[1]];
		v += 2;
	} else {
		c = (v[0]-252)*65536 + v[1]*256 + v[2];
		v += 3;
	}
	if (!--di->rem || v[0]==0) v = 0;
	di->cur = v;
	return c;
}

void decomp_iterator_start(struct decomp_iterator *di, const char *src)
{
	di->src = src;
	di->cur = 0;
	di->rem = 0;
}

wchar_t nfd_iterate(struct nfd_iterator *ci)
{
	uint32_t xc;
	wchar_t wc;
	int ccc;
	/* If there's already a sequence of non-starters where we have
	 * found the desired next ccc in a previous step, iterate characters
	 * until we find the next match with the same ccc or reach the next
	 * starter. */
	if (ci->cur_ccc) {
		do {
			xc = decomp_iterate(&ci->di);
			wc = xc & 0xffffff;
			ccc = xc >> 24;
			if (ccc == ci->cur_ccc) return wc;
		} while (ccc);
	}
	/* Start or resume processing at the decomposition iterator position
	 * just after the last starter, and look for the lowest-ccc character
	 * with ccc at least 1 plus the previously-current ccc. */
	ci->di = ci->di_start;
	int next_ccc = ci->cur_ccc + 1;
	int lowest_greater_ccc = 256;
	wchar_t lowest_ccc_wc = WEOF;
	struct decomp_iterator lowest_ccc_di;
	do {
		xc = decomp_iterate(&ci->di);
		wc = xc & 0xffffff;
		ccc = xc >> 24;
		if (ccc >= next_ccc && ccc < lowest_greater_ccc) {
			lowest_greater_ccc = ccc;
			lowest_ccc_di = ci->di;
			lowest_ccc_wc = wc;
		}
	} while (ccc && ccc != next_ccc);

	/* If we hit the next starter without finding a new ccc,
	 * we're done with the last sequence of non-starters and can
	 * just return the new starter. */
	if (lowest_ccc_wc == WEOF) {
		// clear state
		ci->di_start = ci->di;
		ci->cur_ccc = 0;
		return wc;
	}
	ci->di = lowest_ccc_di;
	ci->cur_ccc = lowest_greater_ccc;
	return lowest_ccc_wc;
}

void nfd_iterator_start(struct nfd_iterator *ci, const char *src)
{
	decomp_iterator_start(&ci->di_start, src);
	ci->di = ci->di_start;
	ci->cur_ccc = 0;
}
