#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#undef NDEBUG
#include <assert.h>

#define MAX_DECOMP 10
#define MAX_CHARMAP 3840

struct ucdinfo {
	unsigned d1, d2;
	unsigned d[MAX_DECOMP];
	unsigned di[MAX_DECOMP];
	unsigned dlen;
	unsigned ccc;
};


int main()
{
	int i;
	unsigned c, ccc, d, d1, d2;
	char line[1000];
	struct ucdinfo *table = calloc(sizeof *table, 0x110000);

	uint16_t ccc_revmap[256];
	uint16_t *char_revmap = calloc(sizeof *char_revmap, 0x110000);
	unsigned *char_freq = calloc(sizeof *char_freq, 0x110000);
	unsigned charmap[MAX_CHARMAP] = { 0 };
	size_t charmap_size = 0;

	/* read in canonical combining classes and decompositions from ucd */
	while (fgets(line, sizeof line, stdin)) {
		assert(strchr(line, '\n'));

		d1 = d2 = 0;
		if (sscanf(line, "%x;%*[^;];%*[^;];%u;%*[^;];%x %x;", &c, &ccc, &d1, &d2) < 2) continue;
		//if (!d2) d1=0; /* hack to suppress singletons, just for counting, don't use */
		//if (d2) d1=0; /* hack to only see singletons, just for counting, don't use */

		assert(c<0x110000);
		assert(d1<0x110000);
		assert(d2<0x110000);
		assert(ccc<256);

		table[c].ccc = ccc;
		table[c].d1 = d1;
		table[c].d2 = d2;
	}

	/* flatten all decompositions */
	for (c=0; c<0x110000; c++) {
		if (!table[c].d1) continue;
		// expand
		for (i=0, d=c; table[d].d1; d=table[d].d1) {
			assert(i < MAX_DECOMP-1);
			table[c].d[i++] = table[d].d2;
		}
		table[c].d[i++] = d;
		// reverse
		for (int j=0; j<i/2; j++) {
			d = table[c].d[j];
			table[c].d[j] = table[c].d[i-1-j];
			table[c].d[i-1-j] = d;
		}
	}

	/* enumerate used ccc values and add them to charmap */
	for (i=0; i<256; i++) ccc_revmap[i] = 0xffff;
	for (c=0; c<0x110000; c++) {
		ccc = table[c].ccc;
		if (ccc_revmap[ccc]!=0xffff) continue;
		assert(charmap_size < MAX_CHARMAP);
		ccc_revmap[ccc] = charmap_size;
		charmap[charmap_size++] = ccc<<24;
	}

	/* compute frequencies of appearance in decompositions, so that except
	 * for non-starters (which need ccc encoded), only characters with
	 * sufficient frquency appear in the map */
	for (c=0; c<0x110000; c++) {
		for (i=0; i<MAX_DECOMP && table[c].d[i]; i++) {
			d = table[c].d[i];
			char_freq[d]++;
		}
	}

	/* enumerate characters used in decompositions. add them
	 * to charmap and encode the decomposition using it. */
	for (i=0; i<0x110000; i++) char_revmap[i] = 0xffff;
	for (c=0; c<0x110000; c++) {
		for (i=0; i<MAX_DECOMP && table[c].d[i]; i++) {
			d = table[c].d[i];
			if (char_freq[d]<3 && !table[d].ccc) {
				table[c].di[i] = (1<<24) + d;
				continue;
			}
			if (char_revmap[d]==0xffff) {
				char_revmap[d] = charmap_size;
				charmap[charmap_size++] = (table[d].ccc<<24)+d;
			}
			table[c].di[i] = char_revmap[d];
		}
	}

	/* convert characters with nonzero ccc to self-maps */
	for (c=0; c<0x110000; c++) {
		ccc = table[c].ccc;
		if (!ccc || table[c].d1) continue;
		table[c].di[0] = ccc_revmap[ccc];
	}

	// show charmap
	if (0) for (i=0; i<charmap_size; i++) {
		printf("%u\t%.4x (%u)\n", charmap[i]>>24, charmap[i]&0xffffff, char_freq[charmap[i]&0xffffff]);
	}

	// non-starter singletons
	if (0) for (c=0; c<0x110000; c++) {
		if (!table[c].d1 || table[c].d2) continue;
		if (table[table[c].d1].ccc)
			printf("%.4x: %.4x %u\n", c, table[c].d1, table[table[c].d1].ccc);
	}

	// list singleton-targets
	if (0) for (c=0; c<0x110000; c++) {
		if (!table[c].d1 || table[c].d2) continue;
		printf("%.5x\n", table[c].d1);
	}
	// list initial uncommon targets
	if (0) for (c=0; c<0x110000; c++) {
		if (!table[c].d[0]) continue;
		if (char_freq[table[c].d[0]] >= 3) continue;
		printf("%.5x\n", table[c].d[0]);
	}

	if (0) for (c=0; c<0x110000; c++) {
		if (!char_freq[c]) continue;
		if (char_freq[c]<3 && table[c].ccc==0) continue;
		printf("%.4x: %u\n", c, char_freq[c]);
	}

	if (0) for (c=0; c<0x110000; c++) {
		if (!table[c].d1) continue;
		printf("%.4x:", c);
		for (int i=0; i<10 && table[c].d[i]; i++)
			printf(" %.4x", table[c].d[i]);
		puts("");
	}

	// compressed decomp mappings
	if (1) for (c=0; c<0x110000; c++) {
		if (!table[c].d[0]) continue;
		printf("%.4x:", c);
		for (int i=0; i<10 && table[c].di[i]; i++) {
			unsigned di = table[c].di[i];
			if (di>>24) {
				printf(" lit_%.4x", di&0xffffff);
			} else if (charmap[di]&0xffffff) {
				printf(" [%u]=%.4x", di, charmap[di]&0xffffff);
				if (charmap[di]>>24)
					printf("(%u)", charmap[di]>>24);
			} else {
				printf(" self(%u)", charmap[di]>>24);
			}
		}
		puts("");
	}
}
