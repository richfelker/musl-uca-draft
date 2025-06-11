#include <stdint.h>
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>

#include "nfd.h"

#define MAXCHARS 16

int main()
{
	struct nfd_iterator ci;

	setlocale(LC_CTYPE, "");

	for (;;) {
		char dummy;
		int i, j, k;
		//while (scanf("%*[#@]%*[^\n]%[\n]", &dummy)==1) printf("[%c]\n", dummy);
		while (scanf("%[#@]",&dummy)==1) {
			scanf("%*[^\n]");
			scanf("%[\n]",&dummy);
		}
		if (feof(stdin)) break;

		wchar_t ws[MAXCHARS+1];
		char s[MAXCHARS*MB_CUR_MAX+1];
		for (i=0; i<MAXCHARS && scanf(" %x ", ws+i)==1; i++);
		ws[i] = 0;
		wcstombs(s, ws, sizeof s);
		scanf(";%*[^;];");
		for (i=0; i<MAXCHARS && scanf(" %x ", ws+i)==1; i++);
		scanf("%*[^\n]%[\n]", &dummy);

		nfd_iterator_start(&ci, s);
		ws[i] = 0;
		for (j=0; j<=i; j++) {
			wchar_t wc = nfd_iterate(&ci);
			if (wc != ws[j]) {
				for (k=0; k<i; k++)
					printf("%.4x ", ws[k]);
				printf(": '%s' : at %d (%lld) got %.4x\n", s, j, (long long)ftello(stdin), wc);
			}
		}
	}
}
