#include <string.h>
#include <limits.h>

#include "collate.h"
#include "lookup.h"

size_t my_strxfrm_l(char *restrict dest, const char *restrict src, size_t n, const unsigned char *collation_root)
{
	/* In absence of collation rules, strcoll behaves as strcmp. */
	if (!collation_root) {
		size_t l = strlen(src);
		if (l < n) memcpy(dest, src, l+1);
		return l;
	}

	char *d = dest;
	int level = 0;
	int reverse = 0;
	size_t total_len = 0;
	size_t cur_len = 0, next_len = 0;
	const unsigned char *flags = ikmlt_lookup(collation_root, 0);
	const unsigned char *hd = ikmlt_lookup(collation_root, 2);
	int nlevels = *flags++;
	int need_next_len = (nlevels > 1 && (flags[1] & 1));
	
	struct ce_iterator ci, ci0;
	ce_iterator_start(&ci, src, 0, collation_root);
	ci0 = ci;

	for (;;) {
		const unsigned char *ce = ce_iterate(&ci);
		if (!ce) {
			// no need for more passes if we can't store output
			if (!d) return total_len;
			if (reverse) {
				d += cur_len;
				reverse = 0;
			}
			if (level+1 < nlevels && (flags[level+1] & 1)) {
				reverse = 1;
				if (d) d += next_len;
			}
			level++;
			if (level == nlevels) break;
			ci = ci0;
			cur_len = next_len;
			next_len = 0;
			need_next_len = (level+1 < nlevels && (flags[level+1] & 1));
			continue;
		}
		const unsigned char *h = ikmlt_lookup(hd, *ce & 0x7f);
		if (h) {
			if (!level) {
				total_len += h[0];
				if (total_len > SIZE_MAX/2) return SIZE_MAX;
				if (total_len >= n) d = 0;
			}
			if (need_next_len)
				next_len += h[2+4*(level+1)+1] + h[2+4*(level+1)+3];
			const unsigned char *k = h + 2+4*level;
			int l = k[1]+k[3];
			if (d) {
				if (reverse) d -= l;
				memcpy(d, h+k[2], k[3]);
				memcpy(d+k[3], ce+1+k[0], k[1]);
				if (!reverse) d += l;
			}
		} else {
			if (!level) {
				total_len += ce[3];
				if (total_len > SIZE_MAX/2) return SIZE_MAX;
				if (total_len >= n) d = 0;
			}
			if (need_next_len)
				next_len += ce[1+level+1]-ce[1+level];
			int o = level ? ce[level] : 0;
			int l = ce[1+level] - o;
			if (d) {
				if (reverse) d -= l;
				memcpy(d, ce+1+nlevels+o, l);
				if (!reverse) d += l;
			}
		}
	}
	*d++ = 0;
	return total_len;
}
