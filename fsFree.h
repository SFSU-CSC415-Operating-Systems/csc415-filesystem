#include "mfs.h"

int init_free(VCB *fs_vcb, int *freespace);
int alloc_free(VCB *fs_vcb, int *freespace, int numberOfBlocks);
int load_free(VCB *fs_vcb, int *freespace);