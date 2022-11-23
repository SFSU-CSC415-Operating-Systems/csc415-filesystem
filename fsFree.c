/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fsFree.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsHelpers.h"

// Initialize the freespace as specified in the file system volume
// control block.
int init_free()
  {
  printf("***** init_free *****\n");
  
  // Calculate the number of blocks needed for the freespace as specified
  // in the vcb.  Then allocate the freespace in memory.
  int num_blocks = get_num_blocks(sizeof(int) * fs_vcb->number_of_blocks,
    fs_vcb->block_size);
  // freespace = malloc(sizeof(int) * fs_vcb->number_of_blocks);

  // Block 0 is the VCB, so the end flag (0xFFFFFFFE) is set for this block.
  freespace[0] = 0xFFFFFFFE;

  // Initialize all other freespace values to the subsequent block
  for (int i = 1; i < (fs_vcb->number_of_blocks); i++)
    {
    freespace[i] = i + 1;
    }

  // Set the last block of the freespace to the end flag.
  freespace[num_blocks] = 0xFFFFFFFE;

  // Set a reference to point to the first free block of the drive
  fs_vcb->freespace_first = num_blocks + 1;
  fs_vcb->freespace_size = num_blocks;

  // Set the number of freespace blocks available (total number of blocks - number
  // of blocks that the freespace occupies - the single block that the VCB
  // occupies)
  fs_vcb->freespace_avail = fs_vcb->number_of_blocks - num_blocks - 1;

  // Set last block of the freespace to the end flag
  freespace[fs_vcb->number_of_blocks - 1] = 0xFFFFFFFE;

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
  
  return 1;  // 1 is the location for the first block of the freespace map
  }

// alloc_free() allocates the numberOfBlocks, and returns the location of the
// first block of the allocation.  This function also sets the reference to the
// first block of the remaining free space to
int alloc_free(int numberOfBlocks)
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
    if (curr == 0xFFFFFFFE)
      {
      perror("Freespace end flag encountered prematurely\n");
      return -1;
      }
    curr = next;
    next = freespace[next];
    }
  printf("Current block to be set to end: %d\n", curr);
  freespace[curr] = 0xFFFFFFFE;
  fs_vcb->freespace_avail -= numberOfBlocks;

  return next;
  }

int restore_free(DE *d_entry)
  {
  printf("***** restore_free *****\n");

  if (d_entry == NULL)
    {
    perror("restore_free: Free space restore failed: d_entry can't be NULL");
    return -1;
    }

  int curr = fs_vcb->freespace_first;
  printf("First freespace block: %d\n", curr);
  int next = freespace[fs_vcb->freespace_first];
  for (int i = 0; i < fs_vcb->freespace_avail - 1; i++)
    {
    if (curr == 0xFFFFFFFE)
      {
      perror("Freespace end flag encountered prematurely\n");
      return -1;
      }
    curr = next;
    next = freespace[next];
    }
  printf("Current block to be set to first block of file/directory ");
  printf("to be restored to free space: %#010lx\n", curr*sizeof(int) + 0x0400);
  freespace[curr] = d_entry->loc;
  fs_vcb->freespace_avail += d_entry->num_blocks;

  return d_entry->num_blocks;
  }

int restore_extra_free(DE *d_entry)
  {
  printf("***** restore_extra_free *****\n");

  if (d_entry == NULL)
    {
    perror("restore_free: Free space restore failed: d_entry can't be NULL");
    return -1;
    }

  // int new_num_blocks = d_entry->size / fs_vcb->block_size + 1;
  int new_num_blocks = get_num_blocks(d_entry->size, fs_vcb->block_size) + 4;
  int num_blocks_to_restore = d_entry->num_blocks - new_num_blocks;
  if (num_blocks_to_restore <= 0)
    {
    return -1;
    }

  d_entry->num_blocks = new_num_blocks;

  int file_last_block = get_block(d_entry->loc, d_entry->num_blocks - 1);

  int curr = fs_vcb->freespace_first;
  printf("First freespace block: %d\n", curr);
  int next = freespace[fs_vcb->freespace_first];
  for (int i = 0; i < fs_vcb->freespace_avail - 1; i++)
    {
    if (curr == 0xFFFFFFFE)
      {
      perror("Freespace end flag encountered prematurely\n");
      return -1;
      }
    curr = next;
    next = freespace[next];
    }

  printf("Current block to be set to first block of empty space of file/directory ");
  printf("to be restored to free space: %#010lx\n", curr*sizeof(int) + 0x0400);
  freespace[curr] = freespace[file_last_block];

  freespace[file_last_block] = 0xFFFFFFFE;

  fs_vcb->freespace_avail += num_blocks_to_restore;

  return num_blocks_to_restore;
  }

int load_free()
  {
  printf("\n***** load_free *****\n");
  int blocks_read = LBAread(freespace, fs_vcb->freespace_size, 1);

  if (blocks_read != fs_vcb->freespace_size)
    {
    perror("LBAread failed\n");
    return -1;
    }

  return blocks_read;
  }

int get_block(long loc, int offset)
  {
  long curr = loc;
  long next = freespace[curr];
  for (int i = 0; i < offset; i++)
    {
    curr = next;
    next = freespace[next];
    }
  return curr;
  }

int get_next_block(long loc)
  {
  return freespace[loc];
  }

void print_free()
  {
  printf("=================== Printing Freespace Map ===================\n");
  printf("Location of start of freespace: %d\n", fs_vcb->freespace_loc);
  printf("Size of freespace in blocks:    %d\n", fs_vcb->freespace_size);
  printf("Number of available blocks:     %d\n", fs_vcb->freespace_avail);
  printf("First free block location:      %d\n", fs_vcb->freespace_first);
  printf("Size of int:                    %lu\n\n", sizeof(int));
  printf("================== First Entry of Each Block ==================\n");
  for (int i = 0; i < fs_vcb->freespace_size*fs_vcb->block_size/sizeof(int); i++)
    {
    if (i % (sizeof(int) * 32) == 0)
      {
      printf("%#010x  ", freespace[i]);
      }
    if ((i + 1) % (sizeof(int) * 32 * 8) == 0)
      {
      printf("\n");
      }
    if (i > fs_vcb->number_of_blocks)
      {
      if (i - 1 == fs_vcb->number_of_blocks)
        {
        printf("\n");
        }
      printf("%#010x  ", freespace[i]);
      if ((i + 1) * 8 == 0)
        {
        printf("\n");
        }
      }
    }
  printf("\n");
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