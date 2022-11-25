/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214, 915780793, 921572896, 920838201
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: fsHelpers.h
*
* Description: Interface for Helper Functions
*
**************************************************************/

#include "mfs.h"

int get_num_blocks(int bytes, int block_size);

void write_all_fs(DE *dir_array);

void write_dir(DE *dir_array);

void set_cw_path();

char* get_last_tok(const char *pathname);