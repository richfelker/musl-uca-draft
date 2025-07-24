#ifndef NFD_H
#define NFD_H

#include <stddef.h>
#include <stdint.h>

struct decomp_iterator {
	const void *src;
	const void *cur;
	unsigned char hbuf[6];
	unsigned char wide;
	unsigned char rem;
};

struct nfd_iterator {
	struct decomp_iterator di, di_start;
	int cur_ccc;
};

void decomp_iterator_start(struct decomp_iterator *, const void *, int);
uint32_t decomp_iterate(struct decomp_iterator *);

void nfd_iterator_start(struct nfd_iterator *, const void *, int);
wchar_t nfd_iterate(struct nfd_iterator *);

#endif
