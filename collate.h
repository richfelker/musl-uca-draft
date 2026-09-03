#ifndef COLLATE_H
#define COLLATE_H

#include "nfd.h"

#define MAX_LEVELS 10
#define PREFIX_MAX 8

// header byte FD, one length for each weight level, up to 3 fractional levels
// before data, 3 bytes of data, one byte for each non-primary weight
#define IMPLICIT_MAX (1+MAX_LEVELS+3+3+MAX_LEVELS-1)

struct cm_iterator {
	struct nfd_iterator ni;
	wchar_t pending_wc;
	const unsigned char *cm_root;
	const unsigned char *implicit_rules;
	wchar_t context_buf[PREFIX_MAX];
	int context_pos;
	unsigned char implicit_buf[IMPLICIT_MAX];
};

struct ce_iterator {
	struct cm_iterator mi;
	const unsigned char *ce;
	const unsigned char *hd;
	const unsigned char *flags;
	int nlevels;
	int rem;
};

const unsigned char *ce_iterate(struct ce_iterator *);
void ce_iterator_start(struct ce_iterator *, const void *, int, const unsigned char *);

#endif
