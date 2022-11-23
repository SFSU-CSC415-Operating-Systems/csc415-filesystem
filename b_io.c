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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "fsDir.h"
#include "mfs.h"
#include "fsLow.h"
#include "fsHelpers.h"
#include "fsFree.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
  DE * fi;	      //holds the low level systems file info
  DE * dir_array; // holds the directory array where the file resides
	char * buf;     // buffer for open file
	int bufOff;		  // current offset/position in buffer
	int bufLen;		  // number of bytes in the buffer
	int curBlock;	  // current file system block location
  int blockIndex; // current block index
  int fileIndex;  // index of file in dir_array
  // int numBlocks;  // number of blocks
  int accessMode; // file access mode
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
  printf("********** b_open **********\n");

	if (startup == 0) b_init();  //Initialize our system
    
  DE *dir_array = parsePath(filename);

	char *last_tok = get_last_tok(filename);

	int found_index = get_de_index(last_tok, dir_array);

  // check all invalid cases if no file/directory found
  if (found_index < 0)
    {
    int error_encountered = 0;
    // cannot read only a file that does not exist 
    if (flags & O_RDONLY)
      {
      perror("b_open: read only: file not found\n");
      error_encountered = 1;
      }

    // cannot create a file if the create file is not set
    if (!(flags & O_CREAT))
      {
      perror("b_open: create flag not set; new file cannot be created\n");
      error_encountered = 1;
      }

    if (error_encountered)
      {
      free(dir_array);
      dir_array = NULL;
      free(last_tok);
      last_tok = NULL;
      
      return -2;
      }
    }
  else
    {
    // check if the DE is of a directory
    if (dir_array[found_index].attr == 'd')
      {
      printf("b_open: '%s' is a directory and cannot be opened as a file\n",
            dir_array[found_index].name);

      free(dir_array);
      dir_array = NULL;
      free(last_tok);
      last_tok = NULL;
        
      return -2;
      }

    // // check if this DE is available
    // if (!(dir_array[found_index].attr == 'f'))
    //   {
    //   printf("b_open: something catastrophic happened: ");
    //   printf("directory array index %d is not available\n",
    //         found_index);
      
    //   free(dir_array);
    //   dir_array = NULL;
    //   free(last_tok);
    //   last_tok = NULL;
      
    //   return -2;
    //   }
    }

	char *buf = malloc(B_CHUNK_SIZE);

	if (buf == NULL)
		{
    perror("b_open: buffer malloc failed\n");
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    
		return (-1);
		}
	
	b_io_fd returnFd = b_getFCB();	// get our own file descriptor
									                // check for error - all used FCB's
  if (returnFd == -1)
    {
    perror("b_open: no free file control blocks available\n");
    
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    
    return returnFd;
    }

  // malloc file directory entry
  fcbArray[returnFd].fi = malloc(sizeof(DE));

  if (fcbArray[returnFd].fi == NULL)
    {
    perror("b_open: fcbArray file info malloc failed\n");
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    
    return (-1);
    }

  // if a file was found, load the directory entry into the fcbArray; otherwise,
  // create a new file
  if (found_index > -1)
    {
    memcpy(fcbArray[returnFd].fi, &dir_array[found_index], sizeof(DE));
    fcbArray[returnFd].fileIndex = found_index;
    }
  else
    {
    int new_file_index = get_avail_de_idx(dir_array);
    printf("New file index: %d\n", new_file_index);
    if (new_file_index == -1)
      {
      return -1;
      }
    
    int new_file_loc = alloc_free(DEFAULT_FILE_BLOCKS);
    if (new_file_loc == -1)
      {
      return -1;
      }

    // New directory entry initialization
    dir_array[new_file_index].size = 0;
    dir_array[new_file_index].num_blocks = DEFAULT_FILE_BLOCKS;
    dir_array[new_file_index].loc = new_file_loc;
    time_t curr_time = time(NULL);
    dir_array[new_file_index].created = curr_time;
    dir_array[new_file_index].modified = curr_time;
    dir_array[new_file_index].accessed = curr_time;
    dir_array[new_file_index].attr = 'f';
    strcpy(dir_array[new_file_index].name, last_tok);

    // write new empty file to file system
    write_all_fs(dir_array);

    // copy new directory entry to fcbArray file info
    memcpy(fcbArray[returnFd].fi, &dir_array[new_file_index], sizeof(DE));
    fcbArray[returnFd].fileIndex = new_file_index;
    }

  fcbArray[returnFd].dir_array = dir_array;
  fcbArray[returnFd].buf = buf;
  fcbArray[returnFd].bufOff = 0;
  fcbArray[returnFd].bufLen = 0;
  fcbArray[returnFd].blockIndex = 0;
  fcbArray[returnFd].curBlock = fcbArray[returnFd].fi->loc;
  fcbArray[returnFd].accessMode = flags;

  if (flags & O_TRUNC)
    {
    fcbArray[returnFd].fi->size = 0;
    }
  
	free(last_tok);
	last_tok = NULL;
	
	return (returnFd);
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

  int block_offset = offset / B_CHUNK_SIZE;
  if (whence & SEEK_SET)
    {
    fcbArray[fd].curBlock = get_block(fcbArray[fd].fi->loc, block_offset);
    }
  else if (whence & SEEK_CUR)
    {
    fcbArray[fd].curBlock = get_block(fcbArray[fd].curBlock, block_offset);
    }
  else if (whence & SEEK_END)
    {
    fcbArray[fd].curBlock = get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks);
    }

  fcbArray[fd].bufOff = offset % B_CHUNK_SIZE;
  fcbArray[fd].bufLen = 0;
  fcbArray[fd].fi->accessed = time(NULL);
	
	return fcbArray[fd].bufOff; //Change this
	}



// Interface to write function
// Based entirely off of b_read as provided by Prof Bierman code in class
int b_write (b_io_fd fd, char * buffer, int count)
	{
  printf("****** b_write *******\n");
  printf("FIRSTBLOCK: %#010lx   CURRENTBLOCK: %#010x\n", 
    fcbArray[fd].fi->loc * 4 + 0x0400, fcbArray[fd].curBlock * 4 + 0x0400);
	if (startup == 0) b_init();  //Initialize our system

  // should not happen, but checking anyways
	if (fcbArray[fd].accessMode & O_RDONLY)
	{
		perror("b_write: file does not have write access");
		return -1;
	}

	// check that fd is between 0 and (MAXFCBS-1) or if count < 0
	if ((fd < 0) || (fd >= MAXFCBS) || count < 0)
		{
		return (-1); 					//invalid file descriptor
		}

  // should not happen, but checking anyways
	if (fcbArray[fd].accessMode & O_RDONLY)
    {
    perror("b_write: file does not have write access");
    return -1;
    }
	
	// check to see if the fcb exists in this location
	if (fcbArray[fd].fi == NULL)
		{
		return -1;
		}

  // print_de(fcbArray[fd].fi);
  // print_fd(fd);

	// check if there is enough space in the final block to contain the buffer
	// copy

  int extra_blocks = get_num_blocks(
    fcbArray[fd].fi->size + count + B_CHUNK_SIZE - (fcbArray[fd].fi->num_blocks * fs_vcb->block_size),
    fs_vcb->block_size);

	if (extra_blocks > 0)
		{
    printf("Extra blocks needed: %d\n", extra_blocks);
    // set the number of newly allocated blocks to the larger of (extra_blocks * 2)
    // or (fcbArray[fd].fi->num_blocks * 2)
		extra_blocks = fcbArray[fd].fi->num_blocks > extra_blocks
                  ? fcbArray[fd].fi->num_blocks : extra_blocks;
    printf("Extra blocks to allocate (more than needed): %d\n", extra_blocks);

    // allocate the extra blocks and save location
    int new_blocks_loc = alloc_free(extra_blocks);
    printf("Location of allocated blocks: %#010x\n", extra_blocks*4 + 0x0400);

    if (new_blocks_loc < 0)
      {
      perror("b_write: freespace allocation failed\n");
      return -1;
      }

    // reset final block of file in the freespace map to the starting block of
    // the newly allocated space
    // printf("File Freespace Map next loc: %#010x\n",
    // freespace[fcbArray[fd].fi->loc + fcbArray[fd].fi->num_blocks - 1]);
    printf("NEXT BLOCK: %#010x\n", get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1));
    freespace[get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1)] = new_blocks_loc;
    printf("NEXT BLOCK: %#010x\n", get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1));
    fcbArray[fd].fi->num_blocks += extra_blocks;
		}

  // if file is empty set variables accordingly
	int availBytesInBuf;
  int bytesDelivered = 0;
  if (fcbArray[fd].fi->size < 1)
    {
    fcbArray[fd].fi->size = 0;
    availBytesInBuf = B_CHUNK_SIZE;
    }
  else
    {
    // available bytes in buffer
    availBytesInBuf = B_CHUNK_SIZE - fcbArray[fd].bufOff;
    }

  printf("\nWrite count: %d\n", count);
  printf("Available bytes in buffer: %d\n", availBytesInBuf);
  printf("Bytes Delivered: %d\n", bytesDelivered);
	
	int part1, part2, part3, numBlocksToCopy, blocksWritten;
	if (availBytesInBuf >= count)
		{
    // printf("availableBytesInfBuf >= count\n");
    // part1 is the part1 section of the data
    // it is the amount of bytes that will fill up the available bytes in buffer
    // if the amount of data is less than the remaining amount in the buffer, we
    // just copy the entire amount into the buffer.
    part1 = count;
    part2 = 0;
    part3 = 0;
		}
  else
    {
    // the file is too big, so the part1 section is just what is left in buffer
    part1 = availBytesInBuf;
    // printf("  part1: %d\n", part1);

    // set the part3 section to all the bytes left in the file
    part3 = count - availBytesInBuf;
    // printf("  part3: %d\n", part3);

    // divide the part3 section by the chunk size to get the total number of
    // complete blocks to copy and multiply by the chunk size to get the bytes
    // that those blocks occupy
    numBlocksToCopy = part3 / B_CHUNK_SIZE;
    // printf("  numBlocksToCopy: %d\n", numBlocksToCopy);

    part2 = numBlocksToCopy * fs_vcb->block_size;
    // printf("  part2: %d\n", part2);

    // finally subtract the complete bytes in part2 from the total bytes left to
    // get the part3 bytes to put in buffer
    part3 = part3 - part2;
    }

  printf("  part1: %d,    part2: %d,    part3: %d\n", part1, part2, part3);

  // memcopy part1 section
  if (part1 > 0)
    {
    char newbuf[part1 + 1];
    memcpy(newbuf, buffer, part1);
    newbuf[part1] = '\0';
    printf("\nPART1 BUFFER:\n%s\n\n", newbuf);
    memcpy(fcbArray[fd].buf + fcbArray[fd].bufOff, buffer, part1);
    fcbArray[fd].bufOff += part1;
    printf("*********** BUF OFFSET : %d\n", fcbArray[fd].bufOff);
    blocksWritten = LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);

  
    // if (part1 + part2 + part3 == part1)
    //   {
    //   blocksWritten = LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);
    //   }
    // if buffer full, write to disk
    if (fcbArray[fd].bufOff >= B_CHUNK_SIZE)
      {
      // if (fcbArray[fd].blockIndex == fcbArray[fd].fi->num_blocks - 1)
      //   {
      //   printf("Extra blocks needed:\n");
      //   // set the number of newly allocated blocks to the larger of (extra_blocks * 2)
      //   // or (fcbArray[fd].fi->num_blocks * 2)
      //   int extra_blocks = fcbArray[fd].fi->num_blocks > extra_blocks
      //                 ? fcbArray[fd].fi->num_blocks : extra_blocks;
      //   printf("Extra blocks to allocate (more than needed): %d\n", extra_blocks);

      //   // allocate the extra blocks and save location
      //   int new_blocks_loc = alloc_free(extra_blocks);
      //   printf("Location of allocated blocks: %#010x\n", extra_blocks*4 + 0x0400);

      //   if (new_blocks_loc < 0)
      //     {
      //     perror("b_write: freespace allocation failed\n");
      //     return -1;
      //     }

      //   // reset final block of file in the freespace map to the starting block of
      //   // the newly allocated space
      //   // printf("File Freespace Map next loc: %#010x\n",
      //   // freespace[fcbArray[fd].fi->loc + fcbArray[fd].fi->num_blocks - 1]);
      //   printf("NEXT BLOCK: %#010x\n", get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1));
      //   freespace[get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1)] = new_blocks_loc;
      //   printf("NEXT BLOCK: %#010x\n", get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1));
      //   fcbArray[fd].fi->num_blocks += extra_blocks;
      //   }

      // blocksWritten = LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);
      // printf("CURRENTBLOCK: %#010x    ", fcbArray[fd].curBlock * 4 + 0x0400);
      fcbArray[fd].curBlock = get_next_block(fcbArray[fd].curBlock);
      // printf("NEWCURRENTBLOCK: %#010x\n", fcbArray[fd].curBlock * 4 + 0x0400);
      fcbArray[fd].bufOff = 0;
      }
    }

  // LBAread all the complete blocks into the buffer
  if (part2 > 0)
    {
    blocksWritten = LBAwrite(buffer + part1, numBlocksToCopy, fcbArray[fd].curBlock);
    fcbArray[fd].curBlock = get_block(fcbArray[fd].curBlock, numBlocksToCopy);
    part2 = blocksWritten * fs_vcb->block_size;
    }

  // NEED TO HANDLE BLOCK RETURNS VS BYTE RETURNS
  // LBAread remaining block into the fcb buffer, and reset buffer offset
  if (part3 > 0)
    {
    printf("BUFFER OFFSET: %d\n", fcbArray[fd].bufOff);
    char newbuf2[part3 + 1];
    memcpy(newbuf2, buffer + part1 + part2, part3);
    newbuf2[part3] = '\0';
    printf("\nPART3 BUFFER:\n%s\n\n", newbuf2);
    // copy the buffer into the file buffer
    memcpy(fcbArray[fd].buf + fcbArray[fd].bufOff, buffer + part1 + part2, part3);
    // fcbArray[fd].bufOff += part3;
    blocksWritten = LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);
    fcbArray[fd].bufOff += part3;
    }

  if (fcbArray[fd].bufOff >= B_CHUNK_SIZE)
    {
    // blocksWritten = LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);
    fcbArray[fd].curBlock = get_next_block(fcbArray[fd].curBlock);
    fcbArray[fd].bufOff = 0;
    }

  time_t cur_time = time(NULL);
  fcbArray[fd].fi->accessed = cur_time;
  fcbArray[fd].fi->modified = cur_time;
  bytesDelivered = part1 + part2 + part3;
  fcbArray[fd].fi->size += bytesDelivered;

  printf("bytes delivered: %d\n\n", bytesDelivered);

  // memcpy(&fcbArray[fd].dir_array[fcbArray[fd].fileIndex], fcbArray[fd].fi, sizeof(DE));

  write_dir(fcbArray[fd].dir_array);

  print_fd(fd);

  return part1 + part2 + part3;
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+

// Based entirely off of Prof Bierman code shared in class
int b_read (b_io_fd fd, char * buffer, int count)
	{
  printf("\n******* b_read *******\n");

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1) or if count < 0
	if ((fd < 0) || (fd >= MAXFCBS)) // || count < 0
    {
    return (-1); //invalid file descriptor
    }

	if (fcbArray[fd].accessMode & O_WRONLY)
    {
    perror("b_read: file does not have read access");
    return -1;
    }
	
	// check to see if the fcb exists in this location
	if (fcbArray[fd].fi == NULL)
		{
		return -1;
		}

  print_fd(fd);

	// available bytes in buffer
	int availBytesInBuf = fcbArray[fd].bufLen - fcbArray[fd].bufOff;
  // printf("Available bytes in buffer: %d\n", availBytesInBuf);

	// number of bytes already delivered
	int bytesDelivered = (fcbArray[fd].blockIndex * B_CHUNK_SIZE) - availBytesInBuf;
  printf("Bytes Delivered: %d\n", bytesDelivered);

	// limit count to file length
  printf("Count: %d   bytesDelivered: %d   File Size: %ld   count+bytes>fiSize: %d\n", count, 
    bytesDelivered, fcbArray[fd].fi->size, ((count + bytesDelivered) > fcbArray[fd].fi->size));
	if ((count + bytesDelivered) > fcbArray[fd].fi->size)
		{
		count = fcbArray[fd].fi->size - bytesDelivered;
    printf("Updated Count: %d\n", count);

		if (count < 0)
			{
			printf("Error: Count: %d   Delivered: %d   CurBlock: %d", count,
							bytesDelivered, fcbArray[fd].curBlock);
			}
		}
	
	int part1, part2, part3, numBlocksToCopy, blocksRead;
	if (availBytesInBuf >= count)
		{
    // part1 is the part1 section of the data
    // it is the amount of bytes that will fill up the available bytes in buffer
    // if the amount of data is less than the remaining amount in the buffer, we
    // just copy the entire amount into the buffer.
    part1 = count;
    part2 = 0;
    part3 = 0;
    printf("A  part1: %d,    part2: %d,    part3: %d\n", part1, part2, part3);
		}
  else
    {
    // the file is too big, so the part1 section is just what is left in buffer
    part1 = availBytesInBuf;

    // set the part3 section to all the bytes left in the file
    part3 = count - availBytesInBuf;

    // divide the part3 section by the chunk size to get the total number of
    // complete blocks to copy and multiply by the chunk size to get the bytes
    // that those blocks occupy
    numBlocksToCopy = part3 / B_CHUNK_SIZE;
    part2 = numBlocksToCopy * fs_vcb->block_size;

    // finally subtract the complete bytes in part2 from the total bytes left to
    // get the part3 bytes to put in buffer
    part3 = part3 - part2;
    printf("B  part1: %d,    part2: %d,    part3: %d\n", part1, part2, part3);
    }

  // memcopy part1 section
  if (part1 > 0)
    {
    memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].bufOff, part1);
    char pbuf[part1 + 1];
    memcpy(pbuf, fcbArray[fd].buf + fcbArray[fd].bufOff, part1);
    pbuf[part1] = '\0';
    printf("PART1 BUFFER:\n%s\n", pbuf);

    fcbArray[fd].bufOff += part1;
    }

  // LBAread all the complete blocks into the buffer
  if (part2 > 0)
    {
    blocksRead = LBAread(buffer + part1, numBlocksToCopy, fcbArray[fd].curBlock) * fs_vcb->block_size;
    fcbArray[fd].blockIndex += numBlocksToCopy;
    part2 = blocksRead * B_CHUNK_SIZE;
    }

  // LBAread remaining block into the fcb buffer, and reset buffer offset
  if (part3 > 0)
    {
    blocksRead = LBAread(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);
    fcbArray[fd].bufLen = B_CHUNK_SIZE;
    // fcbArray[fd].bufLen = fcbArray[fd].fi->size < (fcbArray[fd].blockIndex + 1) * B_CHUNK_SIZE
    //   ? fcbArray[fd].fi->size % B_CHUNK_SIZE : B_CHUNK_SIZE;

    fcbArray[fd].curBlock = get_next_block(fcbArray[fd].curBlock);
    fcbArray[fd].blockIndex += 1;
    fcbArray[fd].bufOff = 0;

    // if the blocksRead is less than what is left in the calculated amount,
    // reset part3 to the smaller amount
    if (fcbArray[fd].bufLen < part3)
      {
      part3 = fcbArray[fd].bufLen;
      }
    
    // if the number of bytes is more than zero, copy the fd buffer to the
    // buffer and set the offset to the position after the data amount.
    if (part3 > 0)
      {
      memcpy(buffer + part1 + part2, fcbArray[fd].buf + fcbArray[fd].bufOff, part3);
      fcbArray[fd].bufOff += part3;
      }
    }

  fcbArray[fd].fi->accessed = time(NULL);
  
  printf("Delivered Bytes for readcnt: %d\n", part1+part2+part3);

  return part1 + part2 + part3;
	}

int b_move (char *dest, char *src)
  {

  DE *src_dir = parsePath(src);
	char *src_tok = get_last_tok(src);
	int src_index = get_de_index(src_tok, src_dir);

  if (src_index < 0)
    {
    perror("b_move: source file/directory not found");
    return -1;
    }

  DE *dest_dir = parsePath(dest);
	char *dest_tok = get_last_tok(dest);
	int dest_index = get_de_index(dest_tok, dest_dir);

  if (dest_index > -1)
    {
    perror("b_move: file/directory with that name already exists");
    return -1;
    }

  if (dest_dir[0].loc == src_dir[0].loc)
    {
    strcpy(src_dir[src_index].name, dest_tok);
    write_dir(src_dir);

    free(src_dir);
    src_dir = NULL;
    free(src_tok);
    src_tok = NULL;

    free(dest_dir);
    dest_dir = NULL;
    free(dest_tok);
    dest_tok = NULL;

    return 0;
    }

  dest_index = get_avail_de_idx(dest_dir);

  if (dest_index < 0)
    {
    return -1;
    }

  memcpy(&dest_dir[dest_index], &src_dir[src_index], sizeof(DE));
  strcpy(dest_dir[dest_index].name, dest_tok);
  write_dir(dest_dir);

  src_dir[src_index].name[0] = '\0';
  src_dir[src_index].attr = 'a';
  write_dir(src_dir);

  free(src_dir);
  src_dir = NULL;
  free(src_tok);
  src_tok = NULL;

  free(dest_dir);
  dest_dir = NULL;
  free(dest_tok);
  dest_tok = NULL;

  return 0;
  }
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
  if (!(fcbArray[fd].accessMode & O_RDONLY) && fcbArray[fd].bufOff > 0)
    LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].curBlock);
    
  memcpy(&fcbArray[fd].dir_array[fcbArray[fd].fileIndex], fcbArray[fd].fi, sizeof(DE));

  write_all_fs(fcbArray[fd].dir_array);
  
  free(fcbArray[fd].dir_array);
  fcbArray[fd].dir_array = NULL;
  free(fcbArray[fd].fi);
  fcbArray[fd].fi = NULL;
  free(fcbArray[fd].buf);
  fcbArray[fd].buf = NULL;
	}

// int assess_block_count(b_io_fd fd, int count)
//   {
//   int extra_blocks = get_num_blocks(
//     fcbArray[fd].fi->size + count - (fcbArray[fd].fi->num_blocks * fs_vcb->block_size),
//     fs_vcb->block_size);

// 	if (extra_blocks > 0)
// 		{
//     printf("Extra blocks needed: %d\n", extra_blocks);
//     // set the number of newly allocated blocks to the larger of (extra_blocks * 2)
//     // or (fcbArray[fd].fi->num_blocks * 2)
// 		extra_blocks = fcbArray[fd].fi->num_blocks > extra_blocks
//                   ? fcbArray[fd].fi->num_blocks : extra_blocks;
//     printf("Extra blocks to allocate (more than needed): %d\n", extra_blocks);

//     // allocate the extra blocks and save location
//     int new_blocks_loc = alloc_free(extra_blocks);
//     printf("Location of allocated blocks: %#010x\n", extra_blocks*4 + 0x0400);

//     if (new_blocks_loc < 0)
//       {
//       perror("b_write: freespace allocation failed\n");
//       return -1;
//       }

//     // reset final block of file in the freespace map to the starting block of
//     // the newly allocated space
//     // printf("File Freespace Map next loc: %#010x\n",
//     // freespace[fcbArray[fd].fi->loc + fcbArray[fd].fi->num_blocks - 1]);
//     printf("NEXT BLOCK: %#010x\n", get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1));
//     freespace[get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1)] = new_blocks_loc;
//     printf("NEXT BLOCK: %#010x\n", get_block(fcbArray[fd].fi->loc, fcbArray[fd].fi->num_blocks - 1));
//     fcbArray[fd].fi->num_blocks += extra_blocks;
// 		}
//   }

int print_fd(b_io_fd fd)
  {
  if (fd < 0)
    {
    return -1;
    }
  printf("=================== Printing FCB Entry ===================\n");
  printf("Index: %02d  Filename: %-10s  FileSize: %-8ld  NumBlocks: %d   bufOff: %04d   bufLen: %04d   curBlk: %#010x\n",
    fd, fcbArray[fd].fi->name, fcbArray[fd].fi->size, fcbArray[fd].fi->num_blocks, fcbArray[fd].bufOff, fcbArray[fd].bufLen, fcbArray[fd].curBlock * 4 + 0x0400);
  }