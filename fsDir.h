#include "mfs.h"

int init_dir(int parent_loc);

void print_dir(DE* dir_array);

void print_de(DE *d_entry);

int get_de_index(char *token, DE* dir_array);

int get_avail_de_idx(DE* dir_array);

int is_empty(DE *d_entry);

// Returns a directory (an array of directory entries)
DE* parsePath(const char *path);
