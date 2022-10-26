#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fsFree.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsHelpers.h"

// freespace variable declared in header "fsFree.h"

// init_free() initializes the freespace as specified in the file system volume
// control block.
int init_free()
    {
    printf("***** init_freespace *****\n");
    
    // Calculate the number of blocks needed for the freespace as specified
    // in the vcb.  Then allocate the freespace in memory.
    int blocks_needed = get_num_blocks(sizeof(int) * fs_vcb->number_of_blocks,
        fs_vcb->block_size);
    freespace = malloc(sizeof(int) * fs_vcb->number_of_blocks);

    // Block 0 is the VCB, so the end flag (0xFFFE) is set for this block.
    freespace[0] = 0xFFFE;

    // Set the freespace location to the next block after the VCB.
    // FREESPACE LOC to the first freespace block? Or should I add a bunch of
    // freespace data in the VCB to track information about the freespace?
    // (e.g. freespace size, beginning of actual free space, number of blocks
    // in freespace, number of blocks left in freespace, contiguous blocks...)
    fs_vcb->freespace_loc = 1;

    // Set the last block of the freespace to the end flag.
    freespace[blocks_needed] = 0xFFFE;
    
    // Initialize all other freespace values to the subsequent block
    for (int i = blocks_needed + 1; i < fs_vcb->number_of_blocks - 1; i++)
        {
        freespace[i] = i + 1;
        }

    // Set a reference to point to the first free block of the drive
    fs_vcb->freespace_first = blocks_needed + 1;

    // Set last block of the freespace to the end flag
    freespace[fs_vcb->number_of_blocks - 1] = 0xFFFE;
    
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

// alloc_free() allocates the numberOfBlocks, and returns the location of the
// first block of the allocation.  This function also sets the reference to the
// first block of the remaining free space to
int alloc_free(int numberOfBlocks)
    {
    if (numberOfBlocks < 1)
        {
        perror("Cannot allocate zero blocks");
        return -1;
        }
    
    if (blocks_unavailable(numberOfBlocks))
        return -1;

    int loc = fs_vcb->freespace_first;
    int curr = fs_vcb->freespace_first;
    int next = freespace[fs_vcb->freespace_first];
    numberOfBlocks--;
    while (numberOfBlocks > 0)
        {
        curr = next;
        next = freespace[next];
        numberOfBlocks--;
        }
    freespace[curr] = 0xFFFE;
    fs_vcb->freespace_first = next;

    return loc;
    }

int blocks_unavailable(int numberOfBlocks)
    {
    int curr = fs_vcb->freespace_first;
    int next = freespace[fs_vcb->freespace_first];
    numberOfBlocks--;
    while (numberOfBlocks > 0)
        {
        curr = next;
        next = freespace[next];
        numberOfBlocks--;
        if (next == 0xFFFE && numberOfBlocks > 0)
            {
            perror("Not enough freespace available.\n");
            printf("Need %d more blocks", numberOfBlocks);
            return numberOfBlocks;
            }
        }
    return numberOfBlocks;
    }