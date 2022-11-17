/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: mfs.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"

#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

#define DE_COUNT 64		  // initial number of d_entries to allocate to a directory
#define MAX_PATH_LENGTH 1024 // initial path length

// This is the directory entry structure for the file system
// This struct is exactly 128 bytes in size
typedef struct
	{
	time_t created;						// time file was created
	time_t modified;					// time file was last modified
	time_t accessed;					// time file was last accessed
	long loc;									// block location of file
	unsigned long size;				// size of the file in bytes
	unsigned int num_blocks;	// number of blocks
	char name[83];						// name of file
	// attributes of file ('d': directory, 'f': file, 'a': available)
	unsigned char attr;
	} DE;

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct
	{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	unsigned short  d_reclen;		/*length of this record getNumBlocks() from line 19 fsDir.c*/
	unsigned short	dirEntryPosition;	/*which directory entry position, like file pos 
																			(index of dir from directory array)*/
	uint64_t	directoryStartLocation;		/*Starting LBA of directory (DE loc) */
	struct fs_diriteminfo *diriteminfo; // pointer to diriteminfo struct
	unsigned int cur_item_index;				// current item index for tracking readdir loc
	} fdDir;

// This is the volume control block structure for the file system
typedef struct
	{
	int number_of_blocks;	// number of blocks in the file system
	int block_size;			// size of each block in the file system
	int freespace_loc;		// location of the first block of the freespace map
	int freespace_first;	// reference to the first free block in the drive
	int freespace_avail;	// number of blocks available in freespace
	int freespace_size;		// number of blocks that freespace occupies
	int root_loc;			// block location of root
	int root_blocks;		// number of blocks the root directory occupies
	long magic;				// unique volume identifier
	} VCB;

extern VCB *fs_vcb;				// volume control block
extern int *freespace;		// freespace map
extern char *cw_path;			// path string
extern DE *cw_dir_array;  // directory structure (array of DEs)

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file


// This is the structure that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;  /* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;  /* time of last status change */
	
	/* add additional attributes here for your file system */
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

