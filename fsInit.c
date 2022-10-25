/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: 
* Student IDs:
* GitHub Name:
* Group Name:
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "fsFree.h"

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */

	// malloc space for the VCB that will take up an entire block
	fs_vcb = malloc(blockSize);

	// read in the volume control block from the first block of file system
	LBAread(fs_vcb, 1, 0);

	// check if the volume control block is the one we are looking for
	if (fs_vcb->magic == 678267415)
		{
		// Do something
		}
	else
		{
		// typedef struct
		// 	{
		// 	int number_of_blocks;
		// 	int block_size;
		// 	int fs_loc;
		// 	int root_loc;
		// 	long magic;
		// 	} VCB;

		fs_vcb->magic = 678267415;
		fs_vcb->number_of_blocks = numberOfBlocks;
		fs_vcb->block_size = blockSize;
		fs_vcb->freespace_loc = init_freespace(fs_vcb);
		}



	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}