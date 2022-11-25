/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214, 915780793, 921572896, 920838201
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: fsFree.c
*
* Description: Free Space Map Operations
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

  return 1;  // 1 is the location for the first block of the freespace map
  }

// alloc_free() allocates the numberOfBlocks, and returns the location of the
// first block of the allocation.  This function also sets the reference to the
// first block of the remaining free space to
int alloc_free(int numberOfBlocks)
  {
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
  
  // find the last free space block minus the number of blocks we need (minus one
  // more).  this block is the new final block of the free space; its contents is
  // the index of the first block of the newly allocated space.
  int curr = fs_vcb->freespace_first;
  int next = freespace[fs_vcb->freespace_first];
  for (int i = 0; i < fs_vcb->freespace_avail - numberOfBlocks - 1; i++)
    {
    if (curr == 0xFFFFFFFE)
      {
      perror("Freespace end flag encountered prematurely\n");
      return -1;
      }

    // current block (at the end of the loop, this is the new final block of the
    // free space)
    curr = next;

    // next block (at the end of this loop, this is the beginning block of the
    // newly allocated space)
    next = freespace[next];
    }

  // set the final block of the free space map
  freespace[curr] = 0xFFFFFFFE;
  fs_vcb->freespace_avail -= numberOfBlocks; // reduce available free space

  // return the index of the first block of the newly allocated space
  return next;
  }

// this function restores all the free space for a file/directory; one step for
// deleting files/directories.  returns the number of blocks returned to free space.
int restore_free(DE *d_entry)
  {
  if (d_entry == NULL)
    {
    perror("restore_free: Free space restore failed: d_entry can't be NULL");
    return -1;
    }

  // find the last block of the free space
  int curr = fs_vcb->freespace_first;
  int next = freespace[fs_vcb->freespace_first];
  for (int i = 0; i < fs_vcb->freespace_avail - 1; i++)
    {
    if (curr == 0xFFFFFFFE)
      {
      perror("Freespace end flag encountered prematurely\n");
      return -1;
      }
    
    // current block (at the end of this loop, this is the final block of the
    // free space)
    curr = next;
    next = freespace[next];
    }

  // change the final block of the available free space to the beginning of the
  // file/directory being deleted, which returns all of the space allocated to
  // the file/directory back to the free space.
  freespace[curr] = d_entry->loc;

  // increase available free space
  fs_vcb->freespace_avail += d_entry->num_blocks;

  return d_entry->num_blocks;
  }

// this function restores extraneous empty blocks of a file/directory back to
// free space.
int restore_extra_free(DE *d_entry)
  {
  if (d_entry == NULL)
    {
    perror("restore_free: Free space restore failed: d_entry can't be NULL");
    return -1;
    }

  // calculate the number of empty blocks to restore.  if the number of empty
  // blocks is 4 or less, just leave the file as-is
  int new_num_blocks = get_num_blocks(d_entry->size, fs_vcb->block_size) + 4;
  int num_blocks_to_restore = d_entry->num_blocks - new_num_blocks;
  if (num_blocks_to_restore <= 0)
    {
    return -1;
    }

  // change the number of blocks to the new updates amount
  d_entry->num_blocks = new_num_blocks;

  // retrieve the new last block of the file/directory
  int file_last_block = get_block(d_entry->loc, d_entry->num_blocks - 1);

  // iterate through the free space to find the last block
  int curr = fs_vcb->freespace_first;
  int next = freespace[fs_vcb->freespace_first];
  for (int i = 0; i < fs_vcb->freespace_avail - 1; i++)
    {
    if (curr == 0xFFFFFFFE)
      {
      perror("Freespace end flag encountered prematurely\n");
      return -1;
      }

    // final block of the free space
    curr = next;
    next = freespace[next];
    }

  // set the final block of free space to the block index of the block following
  // the last block in the file
  freespace[curr] = freespace[file_last_block];

  // set the final block of the file to the end flag
  freespace[file_last_block] = 0xFFFFFFFE;

  // add the number of blocks restored back to the free space available
  fs_vcb->freespace_avail += num_blocks_to_restore;

  return num_blocks_to_restore;
  }

// this function loads the free space map on the drive into memory
int load_free()
  {
  int blocks_read = LBAread(freespace, fs_vcb->freespace_size, 1);

  if (blocks_read != fs_vcb->freespace_size)
    {
    perror("LBAread failed\n");
    return -1;
    }

  return blocks_read;
  }

// this function retrieves the block location at an offset from the block
// location provided
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

// this function retrieves the block location following the location provided
int get_next_block(long loc)
  {
  return freespace[loc];
  }

// print the free space map for debugging
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