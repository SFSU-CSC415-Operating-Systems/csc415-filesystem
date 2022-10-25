#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fsFree.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsHelpers.h"

int get_num_blocks(int bytes, int block_size);

int init_freespace(VCB *vcb)
    {
    printf("***** init_freespace *****\n");
    
    // Calculate the number of blocks needed for the freespace as specified
    // in the vcb.  Then allocate the freespace in memory.
    int blocks_needed = get_num_blocks(sizeof(int) * vcb->number_of_blocks,
        vcb->block_size);
    int * freespace = malloc(sizeof(int) * vcb->number_of_blocks);

    // Block 0 is the VCB, so the end flag (0xFFFE) is set for this block.
    freespace[0] = 0xFFFE;

    // Set the freespace location to the next block after the VCB.
    // FREESPACE LOC to the first freespace block? Or should I add a bunch of
    // freespace data in the VCB to track information about the freespace?
    // (e.g. freespace size, beginning of actual free space, number of blocks
    // in freespace, number of blocks left in freespace, contiguous blocks...)
    vcb->freespace_loc = 1;

    // Set the last block of the freespace to the end flag.
    freespace[blocks_needed] = 0xFFFE;
    
    // Initialize all other freespace values to the subsequent block
    for (int i = blocks_needed + 1; i < vcb->number_of_blocks - 1; i++)
        {
        freespace[i] = i + 1;
        }

    // Set last block of the freespace to the end flag
    freespace[vcb->number_of_blocks - 1] = 0xFFFE;
    
    // Write freespace to disk
    int bytes_written = LBAwrite(freespace, blocks_needed, 1);

    // Check if LBAwrite failed
    if (bytes_written != blocks_needed)
        {
        perror("LBAwrite failed\n");
        return -1;
        }

    printf("LBAwrite bytes written: %d", bytes_written);
    
    return 1;
    }