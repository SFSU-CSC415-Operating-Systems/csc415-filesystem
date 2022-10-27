#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fsFree.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsHelpers.h"

// Initialize the freespace as specified in the file system volume
// control block.
int init_free(VCB *fs_vcb, int *freespace)
    {
    printf("***** init_free *****\n");
    
    // Calculate the number of blocks needed for the freespace as specified
    // in the vcb.  Then allocate the freespace in memory.
    int num_blocks = get_num_blocks(sizeof(int) * fs_vcb->number_of_blocks,
        fs_vcb->block_size);
    // freespace = malloc(sizeof(int) * fs_vcb->number_of_blocks);

    // Block 0 is the VCB, so the end flag (0xFFFE) is set for this block.
    freespace[0] = 0xFFFE;

    // Initialize all other freespace values to the subsequent block
    for (int i = 1; i < fs_vcb->number_of_blocks; i++)
        {
        freespace[i] = i + 1;
        }

    // Set the last block of the freespace to the end flag.
    freespace[num_blocks] = 0xFFFE;

    // Set a reference to point to the first free block of the drive
    fs_vcb->freespace_first = num_blocks + 1;
    fs_vcb->freespace_size = num_blocks;

    // Set the number of freespace blocks available (total number of blocks - number
    // of blocks that the freespace occupies - the single block that the VCB
    // occupies)
    fs_vcb->freespace_avail = fs_vcb->number_of_blocks - num_blocks - 1;

    // Set last block of the freespace to the end flag
    freespace[fs_vcb->number_of_blocks - 1] = 0xFFFE;

    // print_free(fs_vcb, freespace);
    
    // Write freespace to disk
    int blocks_written = LBAwrite(freespace, num_blocks, 1);

    // Check if LBAwrite failed
    if (blocks_written != num_blocks)
        {
        perror("LBAwrite failed\n");
        return -1;
        }

    printf("LBAwrite blocks written: %d\n", blocks_written);
    
    return 1;
    }

// alloc_free() allocates the numberOfBlocks, and returns the location of the
// first block of the allocation.  This function also sets the reference to the
// first block of the remaining free space to
int alloc_free(VCB *fs_vcb, int *freespace, int numberOfBlocks)
    {
    printf("***** alloc_free *****\n");

    if (numberOfBlocks < 1)
        {
        perror("Cannot allocate zero blocks\n");
        return -1;
        }
    
    if (fs_vcb->freespace_avail < numberOfBlocks)
        {
        perror("Not enough freespace available.\n");
        printf("Need %d more blocks\n", numberOfBlocks - fs_vcb->freespace_avail);
        return -1;
        }

    int curr = fs_vcb->freespace_first;
    printf("First freespace block: %d\n", curr);
    int next = freespace[fs_vcb->freespace_first];
    for (int i = 0; i < fs_vcb->freespace_avail - numberOfBlocks - 1; i++)
        {
        if (curr == 0xFFFE)
            {
            perror("Freespace end flag encountered prematurely\n");
            return -1;
            }
        curr = next;
        next = freespace[next];
        }
    printf("Current block to be set to end: %d\n", curr);
    freespace[curr] = 0xFFFE;
    fs_vcb->freespace_avail -= numberOfBlocks;

    return next;
    // return 8000;
    }

int load_free(VCB *fs_vcb, int *freespace)
    {
    printf("***** load_free *****\n");
    freespace = malloc(sizeof(int) * fs_vcb->number_of_blocks);
    int blocks_read = LBAread(freespace, fs_vcb->freespace_size, 1);

    if (blocks_read != fs_vcb->freespace_size)
        {
        perror("LBAread failed\n");
        return -1;
        }

    return blocks_read;
    }

void print_free(VCB *fs_vcb, int *freespace)
    {
    printf("=================== Printing Freespace Map ===================\n");
    printf("Location of start of freespace: %d\n", fs_vcb->freespace_loc);
    printf("Size of freespace in blocks:    %d\n", fs_vcb->freespace_size);
    printf("Number of available blocks:     %d\n", fs_vcb->freespace_avail);
    printf("First free block location:      %d\n", fs_vcb->freespace_first);
    printf("Size of int:                    %lu\n", sizeof(int));
    for (int i = 0; i < 160; i++)
        {
        printf("%#06x  ", freespace[i]);
        if ((i + 1) % 16 == 0)
            {
            printf("\n");
            }
        }
    }

/*
THE FOLLOWING FUNCTIONS ARE FOR FRONT ALLOCATION
*/
// int alloc_free(int numberOfBlocks)
//     {
//     if (numberOfBlocks < 1)
//         {
//         perror("Cannot allocate zero blocks");
//         return -1;
//         }
    
//     if (blocks_unavailable(numberOfBlocks))
//         return -1;

//     int loc = fs_vcb->freespace_first;
//     int curr = fs_vcb->freespace_first;
//     int next = freespace[fs_vcb->freespace_first];
//     numberOfBlocks--;
//     while (numberOfBlocks > 0)
//         {
//         curr = next;
//         next = freespace[next];
//         numberOfBlocks--;
//         }
//     freespace[curr] = 0xFFFE;
//     fs_vcb->freespace_first = next;

//     return loc;
//     }

// int blocks_unavailable(int numberOfBlocks)
//     {
//     int curr = fs_vcb->freespace_first;
//     int next = freespace[fs_vcb->freespace_first];
//     numberOfBlocks--;
//     while (numberOfBlocks > 0)
//         {
//         curr = next;
//         next = freespace[next];
//         numberOfBlocks--;
//         if (next == 0xFFFE && numberOfBlocks > 0)
//             {
//             perror("Not enough freespace available.\n");
//             printf("Need %d more blocks", numberOfBlocks);
//             return numberOfBlocks;
//             }
//         }
//     return numberOfBlocks;
//     }