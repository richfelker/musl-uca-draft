# musl libc collation project draft

This repo is a working tree for development of the Unicode
Normalization Form D and Collation Algorithm implementations for
inclusion in musl libc as part of the locale support overhaul, funded
by NLnet and the NGI Zero Core Fund.


## Runtime code

`nfd.c` and `nfd.h` provide an iterator based implementation of
Unicode NFD, using compact tables generated from `UnicodeData.txt`.

`collate.c` and `collate.h` provide an iterator for the sequence of
collation elements associated with an input string.

`lookup.c` and `lookup.h` implement the integer-keyed multi-level
table data structure used for collation and all of the new
memory-mapped locale data image format.


## Normalization table generation

The `build_decomp.c` program takes `UnicodeData.txt` on stdin and
produces as output the contents of `decomp.h` for use by the above
`nfd.c`. The multi-level table in the output maps codepoint values for
decomposable and non-starter characters to fully flattened decomposed
sequence tagged with canonical combining class of each character in
the sequence.


## Testing

`unitest.c` takes the Unicode `NormalizationTest.txt` vectors and
validates that the NFD implementation applied to column 1 (source) of
each test vector matches column 3 (NFD).
