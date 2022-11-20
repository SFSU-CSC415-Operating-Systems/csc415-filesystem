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

typedef struct b_fcb
	{
  DE * fi;	      //holds the low level systems file info
	char * buf;     // buffer for open file
	int bufOff;		  // current offset/position in buffer
	int bufLen;		  // number of bytes in the buffer
	int curBlock;	  // current block number
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
  if (found_index == -1)
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

    // cannot truncate a file if the file does not exist
    if (flags & O_TRUNC)
      {
      perror("b_open: file not found to be truncated\n");
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

  // check if this DE is available
  if (!(dir_array[found_index].attr == 'a'))
    {
    printf("b_open: something catastrophic happened: ");
    printf("directory array index %d is not available\n",
          found_index);
    
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    
    return -2;
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

  if (fcbArray[returnFd].fi)
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
    dir_array[new_file_index].size = DEFAULT_FILE_BLOCKS * fs_vcb->block_size;
    dir_array[new_file_index].num_blocks = DEFAULT_FILE_BLOCKS;
    dir_array[new_file_index].loc = new_file_loc;
    time_t curr_time = time(NULL);
    dir_array[new_file_index].created = curr_time;
    dir_array[new_file_index].modified = curr_time;
    dir_array[new_file_index].accessed = curr_time;
    dir_array[new_file_index].attr = 'f';
    strcpy(dir_array[new_file_index].name, last_tok);

    // write new empty file to file system
    update_all_fs(dir_array);

    // copy new directory entry to fcbArray file info
    memcpy(fcbArray[returnFd].fi, &dir_array[new_file_index], sizeof(DE));
    
    }
  fcbArray[returnFd].buf = buf;
  fcbArray[returnFd].bufOff = 0;
  fcbArray[returnFd].bufLen = 0;
  fcbArray[returnFd].curBlock = 0;
  fcbArray[returnFd].accessMode = flags;
	
	free(dir_array);
	dir_array = NULL;
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
		
		
	return (0); //Change this
	}



// Interface to write function
// Based entirely off of b_read as provided by Prof Bierman code in class
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

  // should not happen, but checking anyways
	if (fcbArray[fd].accessMode & O_RDONLY)
	{
		perror("b_write: file does not have write access");
		return -1;
	}

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
  // check if the file info is NULL which means the file does not exist and a
  // new file needs to be created
  if (fcbArray[fd].fi == NULL)
    {
    if (!(fcbArray[fd].accessMode & O_CREAT))
      {
      perror("b_write: file does not have create access");
      return -1;
      }
    }

	// check if there is enough space in the final block to contain the entire
	// file
	if (fcbArray[fd].fi->size + count > fcbArray[fd].fi->num_blocks * fs_vcb->block_size)
		{
		// allocate more freespace
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

// Based entirely off of Prof Bierman code shared in class
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{

	}

DE *getFileInfo(char *filename)
  {
  
  }
