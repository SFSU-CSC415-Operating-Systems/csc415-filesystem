/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214, 915780793, 921572896, 920838201
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: fsDir.h
*
* Description: Interface for Directory Functions
*
**************************************************************/

#include "mfs.h"

int init_dir(int parent_loc);

void print_dir(DE* dir_array);

void print_de(DE *d_entry);

int get_de_index(char *token, DE* dir_array);

int get_avail_de_idx(DE* dir_array);

int is_empty(DE *d_entry);

// Returns a directory (an array of directory entries)
DE* parsePath(const char *path);
