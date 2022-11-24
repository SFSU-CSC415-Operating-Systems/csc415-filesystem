/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214, 915780793, 921572896, 920838201
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: fsFree.h
*
* Description: Interface for Free Space Map Functions
*
**************************************************************/

#include "mfs.h"

int init_free();
int alloc_free(int numberOfBlocks);
int load_free();
void print_free();
int restore_free(DE *d_entry);
int get_block(long loc, int offset);
int get_next_block(long loc);
int restore_extra_free(DE *d_entry);