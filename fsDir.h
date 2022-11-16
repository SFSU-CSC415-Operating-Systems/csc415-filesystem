#include "mfs.h"

int init_dir(int parent_loc);

void print_dir(DE* dir_array);

void print_de(DE *dir);

char* get_last_tok(char *path);

int get_de_index(char *token, DE* dir_array);

int get_avail_de_idx(DE* dir_array);

// Returns a directory (an array of directory entries)
DE* parsePath(char *pathname);
