

static unsigned get8(const unsigned char *b)
{
	return b[0];
}

static unsigned get16(const unsigned char *b)
{
	return (b[0]<<8) | b[1];
}

static unsigned get32(const unsigned char *b)
{
	return ((unsigned)b[0]<<24) | (b[1]<<16) | (b[2]<<8) | b[3];
}


const void *lookup(const unsigned char *ld, int key)
{
	unsigned shift, scale, cnt, val, x, k = key;

	do {
		k -= get32(ld);
		shift = ld[4];
		scale = ld[5];
		//if (shift > 31 || scale > 2) return 0;

		cnt = get16(ld+6)+1;
		x = k>>shift;
		k &= (1U<<shift) - 1;
		if (x >= cnt) return 0;
		x <<= scale;

		ld += 8;
		for (int i=val=0; i<(1U<<scale); i++)
			val = 256*val + get8(ld+x+i);
		if (!val) return 0;
		ld += (cnt<<scale) + val-1;
	} while (shift);

	return ld;
}

const void *lookup_path(const unsigned char *ld, const int *path)
{
	for (int i=0; i<path[0]; i++) {
		const unsigned char *next = lookup(ld, path[i+1]);
		if (!next) return 0;
		ld = next;
	}
	return ld;
}
