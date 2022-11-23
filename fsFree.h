#include "mfs.h"

int init_free();
int alloc_free(int numberOfBlocks);
int load_free();
void print_free();
int restore_free(DE *d_entry);
int get_block(long loc, int offset);
int get_next_block(long loc);
int restore_extra_free(DE *d_entry);