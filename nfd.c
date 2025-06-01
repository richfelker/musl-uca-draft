#include <stdlib.h>
#include <limits.h>
#include <wchar.h>

struct decomp_iterator {
	const char *src;
	const void *cur;
	size_t rem;
};

struct ccc_iterator {
	struct decomp_iterator di, di_start;
	int cur_ccc;
};

wchar_t decomp_iterate(struct decomp_iterator *di)
{
	const unsigned char *v;
	wchar_t wc;

	if (!di->cur) {
		unsigned b = *di->src;
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

		if (wc>=0x30000) return wc;
		a = toplevel[wc>>6];
		if (a==0xffff) return wc;
		v = secondlevel+a;
		unsigned low = wc & 63;
		unsigned min = v[0] & 63;
		unsigned len = v[1] & 63;
		unsigned max = (v[0]>>6) + (v[1]>>6)*4;
		unsigned start = v[2] + 256*v[3];
		v += 4;
		low -= min;
		if (low > len || !v[low]) return wc;
		di->cur = expansions + start + v[low];
		di->rem = max;
	}
	v = di->cur;
	if (v[0] < 240) {
		c = decomp_map[v[0]];
		if (!(c & 0xffffff))
			c += wc;
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
	return wc;
}

void decomp_iterator_start(struct decomp_iterator *di, const char *src)
{
	di->src = src;
	di->cur = 0;
	di->pos = 0;
}

wchar_t ccc_iterate(struct ccc_iterator *ci)
{
	wchar_t wc;
	int ccc;
	/* If there's already a sequence of non-starters where we have
	 * found the desired next ccc in a previous step, iterate characters
	 * until we find the next match with the same ccc or reach the next
	 * starter. */
	if (ci->cur_ccc) {
		do {
			wc = decomp_iterate(&ci->di);
			ccc = cccof(wc);
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
		wc = decomp_iterate(&ci->di);
		ccc = cccof(wc);
		if (ccc >= next_ccc && ccc < lowest_greater_ccc) {
			lowest_greater_ccc = ccc;
			lowest_ccc_di = ci->di;
			lowest_ccc_wc = wc;
		}
	} while (ccc > next_ccc);

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

void ccc_iterator_start(struct ccc_iterator *ci, const char *src)
{
	decomp_iterator_start(&ci->di_start, src);
	ci->di = ci->di_start;
	ci->cur_ccc = 0;
}
