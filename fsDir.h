#include "mfs.h"

int init_dir(VCB *fs_vcb, int *freespace, int parent_loc);

void print_dir(DE* dir_array);

void print_de(DE *dir);

DE* alloc_dir_array(VCB* fs_vcb);