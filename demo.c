#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>

#include "collate.h"
#include "lookup.h"

static int decode_ce(unsigned char *d, const unsigned char *ce, const unsigned char *h, int level, int nlevels)
{
	if (h) {
		const unsigned char *k = h + 2+4*level;
		memcpy(d, h+k[2], k[3]);
		memcpy(d+k[3], ce+1+k[0], k[1]);
		return k[3] + k[1];
	} else {
		int o = level ? ce[level] : 0;
		memcpy(d, ce+1+nlevels+o, ce[1+level] - o);
		return ce[1+level] - o;
	}
}

size_t my_strxfrm_l(char *restrict dest, const char *restrict src, size_t n, const unsigned char *collation_root);

int main(int argc, char **argv)
{
	setlocale(LC_CTYPE, "");

	int fd = 0;
	off_t len = lseek(fd, 0, SEEK_END);
	if (len < 0) {
		perror("cannot get file size");
		return 1;
	}
	const unsigned char *loc = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (loc == MAP_FAILED) {
		perror("cannot mmap locale file");
		return 1;
	}
	const unsigned char *collation_root = ikmlt_lookup(loc+16, 3);

	struct ce_iterator ci;
	ce_iterator_start(&ci, argv[1], 0, collation_root);
	const unsigned char *hd = ikmlt_lookup(collation_root, 2);
	int nlevels = *(const unsigned char *)ikmlt_lookup(collation_root, 0);

	int i=0;
	const unsigned char *ce;
	while ((ce = ce_iterate(&ci))) {
		printf("%d: [", i++);
		unsigned char buf[100];
		const unsigned char *h = ikmlt_lookup(hd, *ce);
		if (1) for (int k=0; k<nlevels; k++) {
			if (k) printf(", ");
			int l = decode_ce(buf, ce, h, k, nlevels);
			for (int j=0; j<l; j++)
				printf(" %.2x"+!j, buf[j]);
		}
		if (0) for (int j=0; j<10; j++)
			printf(" %.2x", ce[j]);
		printf("]\n");
	}
	char buf[100];
	my_strxfrm_l(buf, argv[1], sizeof buf, collation_root);
	for (int i=0; buf[i]; i++)
		printf(" %.2x"+!i, (unsigned char)buf[i]);
	printf("\n");
}
