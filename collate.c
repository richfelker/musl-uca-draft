#include <wchar.h>
#include <string.h>
#include "collate.h"
#include "lookup.h"

static void buffer_wc(struct cm_iterator *mi, wchar_t wc)
{
	mi->context_buf[mi->context_pos] = wc;
	if (mi->context_pos == PREFIX_MAX-1) mi->context_pos = 0;
	else mi->context_pos++;
}

static int load_uni(const unsigned char *p)
{
	return ((unsigned)p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3]) & 0xffffff;
}

static unsigned char *implicit_ce(struct cm_iterator *mi, wchar_t wc)
{
	unsigned char *buf = mi->implicit_buf;
	const unsigned char *ir;
	for (int i=0; (ir=ikmlt_lookup(mi->implicit_rules, i)); i++) {
		int start = load_uni(ir+0);
		int end = load_uni(ir+4);
		if (wc < start || wc > end) continue;
		int base = load_uni(ir+8);
		wc -= base;
		if (ir[12] != 1) continue;
		int blb = ir[13];
		int po = ir[14], so = ir[16];
		int pl = ir[15], sl = ir[17];
		if (pl+sl+3 > IMPLICIT_MAX) continue;
		memcpy(buf, ir+po, pl);
		memcpy(buf+pl+3, ir+so, sl);
		buf[pl+0] = blb + (wc>>13);
		buf[pl+1] = 0x80 + ((wc>>6) & 0x7f);
		buf[pl+2] = 0x80 + (wc & 0x3f) * 2;
		return buf;
	}
	/* As a last resort, emit a fully-ignorable */
	buf[0] = 0xfd;
	for (int i=0; i<MAX_LEVELS; i++) buf[1+i] = 0;
	return buf;
}

static const unsigned char *cm_iterate(struct cm_iterator *mi)
{
	wchar_t wc = mi->pending_wc;
	mi->pending_wc = 0;
	if (!wc) wc = nfd_iterate(&mi->ni);
	if (!wc) return 0;
	const unsigned char *tmp, *cm = ikmlt_lookup(mi->cm_root, wc);

	if (!cm) cm = implicit_ce(mi, wc);

	int i = mi->context_pos;
	while (*cm == 255) {
		if (i) i--;
		else i=PREFIX_MAX-1;
		tmp = ikmlt_lookup(cm, -(int)mi->context_buf[i]);
		if (!tmp) break;
		cm = tmp;
	}
	buffer_wc(mi, wc);
	while (*cm == 0 || *cm == 255) {
		wc = nfd_iterate(&mi->ni);
		tmp = ikmlt_lookup(cm, wc);
		if (!tmp) {
			mi->pending_wc = wc;
			cm = ikmlt_lookup(cm, 0);
			break;
		}
		buffer_wc(mi, wc);
		cm = tmp;
	}
	return cm;
}

static void cm_iterator_start(struct cm_iterator *mi, const void *src, int wide, const unsigned char *collation_root)
{
	*mi = (struct cm_iterator){0};
	nfd_iterator_start(&mi->ni, src, wide);
	mi->cm_root = ikmlt_lookup(collation_root, 1);
	mi->implicit_rules = ikmlt_lookup(collation_root, 3);
}

const unsigned char *ce_iterate(struct ce_iterator *ci)
{
	const unsigned char *ce = ci->ce;
	if (!ce) {
		ce = cm_iterate(&ci->mi);
		if (!ce || *ce != 0xfe) return ce;
		ci->rem = ce[1]; // fixme: handle bogus 0?
		ce += 2;
	}
	if (--ci->rem) {
		const unsigned char *h = ikmlt_lookup(ci->hd, *ce);
		if (h) ci->ce = ce + 1 + h[1];
		else ci->ce = ce + 4 + ce[ci->nlevels];
	} else {
		ci->rem = 0;
		ci->ce = 0;
	}
	return ce;
}

void ce_iterator_start(struct ce_iterator *ci, const void *src, int wide, const unsigned char *collation_root)
{
	*ci = (struct ce_iterator){0};
	cm_iterator_start(&ci->mi, src, wide, collation_root);
	ci->hd = ikmlt_lookup(collation_root, 2);
	ci->flags = ikmlt_lookup(collation_root, 0);
	ci->nlevels = *ci->flags++;
}
