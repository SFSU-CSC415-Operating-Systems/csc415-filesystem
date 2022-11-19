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

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define FILE_BLOCKS 256

typedef struct b_fcb
	{
    DE * fi;	    //holds the low level systems file info
	char * buf;     // buffer for open file
	int bufOff;		// current offset/position in buffer
	int bufLen;		// number of bytes in the buffer
	int curBlock;	// current block number
	int blockLen;	// number of blocks in file
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
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
    
    DE *dir_array = parsePath(filename);

	char *part3_tok = get_part3_tok(filename);

	int found_index = get_de_index(part3_tok, dir_array);

	char *buf = malloc(B_CHUNK_SIZE);

	if (buf == NULL)
		{
		return (-1);
		}

	if (found_index == -1 && (flags & O_WRONLY || flags & O_RDWR))
		{
		int d_entry_index = get_avail_de_idx(dir_array);

		if (d_entry_index == -1)
			{
			free(dir_array);
			dir_array = NULL;
			free(part3_tok);
			part3_tok = NULL;
			return NULL;
			}

		int new_file_loc = alloc_free(FILE_BLOCKS);
		}
	
	returnFd = b_getFCB();	// get our own file descriptor
									// check for error - all used FCB's
    if (returnFd == -1) 
        {
        perror("No free file control blocks available.");
        return returnFd;
        }

    // Allocate buffer for file
    fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);

    if (fcbArray[returnFd].buf == NULL)
        {
        perror("Could not allocate buffer");
        return EXIT_FAILURE;
        }
  
    // Initialize file descriptor
    fcbArray[returnFd].fi = GetFileInfo(filename);
    fcbArray[returnFd].bufOff = 0;
    fcbArray[returnFd].curBlock = 0;
    fcbArray[returnFd].bufLen = 0;
    fcbArray[returnFd].accessMode = flags;

	
	free(dir_array);
	dir_array = NULL;
	free(part3_tok);
	part3_tok = NULL;
	
	return (returnFd);						// all set
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
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
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
int b_read (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
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

	// available bytes in buffer
	int availBytesInBuf = fcbArray[fd].bufLen - fcbArray[fd].bufOff;

	// number of bytes already delivered
	int bytesDelivered = (fcbArray[fd].curBlock * B_CHUNK_SIZE) - availBytesInBuf;

	// limit count to file length
	if ((count + bytesDelivered) > fcbArray[fd].fi->size)
		{
		count = fcbArray[fd].fi->size - bytesDelivered;

		if (count < 0)
			{
			printf("Error: Count: %d   Delivered: %d   CurBlock: %d", count,
							bytesDelivered, fcbArray[fd].curBlock);
			}
		}
	
	int part1, part2, part3, numBlocksToCopy, bytesRead;
	if (availBytesInBuf >= count)
		{
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
        }

    // memcopy part1 section
    if (part1 > 0)
        {
        memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].bufOff, part1);
        fcbArray[fd].bufOff += part1;
        }

    // LBAread all the complete blocks into the buffer
    if (part2 > 0)
        {
        bytesRead = LBAread(buffer + part1, numBlocksToCopy, 
        fcbArray[fd].curBlock + fcbArray[fd].fi->loc) * fs_vcb->block_size;
        fcbArray[fd].curBlock += numBlocksToCopy;
        part2 = bytesRead;
        }

    // LBAread remaining block into the fcb buffer, and reset buffer offset
    if (part3 > 0)
        {
        bytesRead = LBAread(fcbArray[fd].buf, 1, 
            fcbArray[fd].curBlock + fcbArray[fd].fi->loc) * fs_vcb->block_size;
        fcbArray[fd].curBlock += 1;
        fcbArray[fd].bufOff = 0;
        fcbArray[fd].bufLen = bytesRead;

    // if the bytesRead is less than what is left in the calculated amount,
    // reset part3 to the smaller amount
    if (bytesRead < part3)
      {
      part3 = bytesRead;
      }
    
    // if the number of bytes is more than zero, copy the fd buffer to the
    // buffer and set the offset to the position after the data amount.
    if (part3 > 0)
      {
      memcpy(buffer + part1 + part2, fcbArray[fd].buf + fcbArray[fd].bufOff, 
        fcbArray[fd].curBlock + fcbArray[fd].fi->loc);
      fcbArray[fd].bufOff += part3;
      }
    }

    return part1 + part2 + part3;
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{

	}
