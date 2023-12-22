all: ltils rewrite
rewrite:rewrite.o bitmap.o
	gcc -Wall rewrite.o bitmap.o `pkg-config fuse --cflags --libs` -o  rewrite
ltils:ltils.o bitmap.o
	gcc -Wall ltils.o bitmap.o `pkg-config fuse --cflags --libs` -o  lfs
bitmap.o:hdfslowfs.h bitmap.c
	gcc  -Wall -D_FILE_OFFSET_BITS=64 -g  -c -o bitmap.o bitmap.c
.PHONY : all
clean :
	rm  -f u_fs init_sb_bitmap_data_blocks u_fs.o utils.o init_sb_bitmap_data_blocks.o
