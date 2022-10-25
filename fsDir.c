#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fsDir.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsHelpers.h"

#define DE_COUNT 40		// initial number of d_entries to allocate to a directory

int init_root()
    {
    printf("***** init_root *****\n");

    int blocks_needed = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
    DE* root = malloc(blocks_needed * fs_vcb->block_size);

    // typedef struct
    //     {
    //     int size;			// size of the file
    //     int loc;			// block location of file
    //     time_t created;		// time file was created
    //     time_t modified;	// time file was last modified
    //     time_t accessed;	// time file was last accessed
    //     char attr;			// attributes of file ('d': directory, 'f': file)
    //     char name[43];		// name of file
    //     } DE;

    // root "." entry initialization
    root[0].size = blocks_needed * fs_vcb->block_size;
    root[0].loc = 1

    // next write root ".." entry initialization

    // then write a loop to initialize all other entries to a free state
    for (int i = 0; i < DE_COUNT; i++)
        {
        
        }
    }