#include "mfs.h"

int get_num_blocks(int bytes, int block_size);

void write_all_fs(DE *dir_array);

void write_dir(DE *dir_array);

char* set_cw_path();

char* get_last_tok(const char *pathname);