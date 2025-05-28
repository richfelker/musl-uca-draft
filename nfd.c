


struct decomp_iterator {
	const char *src;
	const void *cur;
	size_t pos;
};

struct ccc_iterator {
	struct decomp_iterator di, di_start, di_min;
	
};



wchar_t decomp_iterate(struct decomp_iterator *di)
{
	if (!*di->cur) {
		wchar_t wc;
		int l = mbtowc(&wc, di->src, MB_LEN_MAX);
		if (l < 0) {
			wc = 0xfffd;
			l = 1;
		}
		di->src += l;
		di->cur = find_decomp(wc);
		di->pos = 0;
	}
}



wchar_t ccc_iterate(struct ccc_iterator *ci)
{
	wchar_t wc;
	if (!in progress) {
		wc = decomp_iterate(&ci->di);
		ccc = cccof(wc);
		if (!ccc) return wc;
		ci->di_start = ci->di;
		ci->cur_ccc = 0;
	}
	if (ci->cur_ccc) {
		do {
			wc = decomp_iterate(&ci->di);
			ccc = cccof(wc);
			if (ccc == ci->cur_ccc) return wc;
		} while (ccc);
	}
	next_ccc = ci->cur_ccc + 1;
	ci->di = ci->di_start;
	int lowest_greater_ccc = 256;
	wchar_t lowest_ccc_wc = WEOF;
	do {
		wc = decomp_iterate(&ci->di);
		ccc = cccof(wc);
		if (ccc >= next_ccc && ccc < lowest_greater_ccc) {
			lowest_greater_ccc = ccc;
			ci->di_min = di;
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
	return lowest_wc;
}
