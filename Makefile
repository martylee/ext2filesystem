CC=gcc
CFLAGS= -Wall -g -o
LIBFLAGS= -Wall -g -c
DEP= ext2_lib.h ext2.h

all:  ext2_ls ext2_rm ext2_mkdir ext2_cp ext2_ln readimage

ext2_ls: ext2_ls.c
	gcc -o ext2_ls ext2_ls.c

ext2_ln: ext2_ln.c
	gcc -o ext2_ln ext2_ln.c

ext2_rm: ext2_rm.c
	gcc -o ext2_rm ext2_rm.c

ext2_cp: ext2_cp.c
	gcc -o ext2_cp ext2_cp.c

ext2_mkdir: ext2_mkdir.c
	gcc -o ext2_mkdir ext2_mkdir.c

readimage: readimage.c
	gcc -Wall -g -o readimage readimage.c -std=C99

clean:
	find . -not -name '*.c' -not -name '*.h' -not -name '*.img' -not -name 'Makefile' -not -path './.git/*' -type f | xargs rm -r
