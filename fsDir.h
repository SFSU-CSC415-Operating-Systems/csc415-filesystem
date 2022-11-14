#include "mfs.h"

int init_dir(int parent_loc);

void print_dir(DE* dir_array);

void print_de(DE *dir);

// allocate a directory
DE* alloc_dir_array();

// Returns a directory (an array of directory entries)
DE* parsePath(char *pathname);