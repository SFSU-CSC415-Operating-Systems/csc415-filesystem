#include "mfs.h"

int init_dir(int parent_loc);

void print_dir(DE* dir_array);

void print_de(DE *dir);

<<<<<<< HEAD
<<<<<<< HEAD
=======
char* get_last_tok(char *path);
=======
char* get_last_tok(const char *pathname);
>>>>>>> origin/features

int get_de_index(char *token, DE* dir_array);

int get_avail_de_idx(DE* dir_array);

<<<<<<< HEAD
>>>>>>> origin/feat_merge
=======
char* set_cw_path();

>>>>>>> origin/features
// Returns a directory (an array of directory entries)
DE* parsePath(const char *path);
