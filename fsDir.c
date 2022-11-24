/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214, 915780793, 921572896, 920838201
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: fsDir.c
*
* Description: Directory Operations
*
**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fsDir.h"
#include "fsLow.h"
#include "fsFree.h"
#include "fsHelpers.h"

// Initialize a directory
// If parent_loc == 0, this is the root directory
int init_dir(int parent_loc)
  {
  // malloc a directory (directory entry array)
  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;
  DE* dir_array = malloc(num_bytes);

  // allocate free space for the directory array
  int dir_loc = alloc_free(num_blocks);
  if (dir_loc == -1)
    {
    perror("Allocation of directory failed.\n");
    free(dir_array);
    dir_array = NULL;
    return -1;
    }

  // Directory "." entry initialization
  dir_array[0].size = num_bytes;
  dir_array[0].num_blocks = num_blocks;
  dir_array[0].loc = dir_loc;
  time_t curr_time = time(NULL);
  dir_array[0].created = curr_time;
  dir_array[0].modified = curr_time;
  dir_array[0].accessed = curr_time;
  dir_array[0].attr = 'd';
  strcpy(dir_array[0].name, ".");

  // Parent directory ".." entry initialization
  // If parent_loc == 0, this is the root directory
  if (parent_loc == 0)
    {
    // track number of blocks root directory occupies
    fs_vcb->root_blocks = num_blocks;
    dir_array[1].size = num_bytes;
    dir_array[1].num_blocks = num_blocks;
    dir_array[1].loc = dir_loc;        // currently the only difference
    dir_array[1].created = curr_time;
    dir_array[1].modified = curr_time;
    dir_array[1].accessed = curr_time;
    }
  else
    {
    // load the parent directory to retrieve parent directory information so
    // that the parent directory information can be filled out in this directory
    // entry array
    DE *parent_dir = malloc(num_bytes);
    if (LBAread(parent_dir, num_blocks, parent_loc) != num_blocks)
      {
      perror("LBAread failed when reading parent directory.\n");
      return -1;
      }
    dir_array[1].size = parent_dir->size;
    dir_array[1].num_blocks = num_blocks;
    dir_array[1].loc = parent_loc;
    dir_array[1].created = parent_dir->created;
    dir_array[1].modified = parent_dir->modified;
    dir_array[1].accessed = parent_dir->accessed;

    free(parent_dir);
    parent_dir = NULL;
    }

  // finish setting the parent directory entry information accordingly
  dir_array[1].attr = 'd';
  strcpy(dir_array[1].name, "..");

  // set all other directory entries to available
  for (int i = 2; i < DE_COUNT; i++)
    {
    dir_array[i].attr = 'a';
    }

  // write new directory to disk
  int blocks_written = LBAwrite(dir_array, num_blocks, dir_loc);

  if (blocks_written != num_blocks)
    {
    perror("LBAwrite failed\n");
    return -1;
    }

  free(dir_array);
  dir_array = NULL;

  // write updated free space map to disk
  if (LBAwrite(freespace, fs_vcb->freespace_size, 1) != fs_vcb->freespace_size)
		{
		perror("LBAwrite failed when trying to write the freespace\n");
		}
          
  return dir_loc;
  }

// parsePath returns NULL if the path is invalid (e.g. one of the directories
// does not exist) or the n-1 directory (array of DE's) pointer
DE* parsePath(const char *path)
  {
  // set a temporary mutable pathname string from the path that is passed in
  char *pathname = malloc(strlen(path) + 1);
  strcpy(pathname, path);

  // malloc a directory (directory entry array)
  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;
  DE* dir_array = malloc(num_bytes);  // malloc space for a directory

  // if the path starts with '/' it is an absolute path, so the root directory
  // must be loaded. otherwise, it is a relative path and the current working
  // directory is copied to the dir_array
  if (pathname[0] == '/')
    {
    LBAread(dir_array, fs_vcb->root_blocks, fs_vcb->root_loc);
    }
  else
    {
    memcpy(dir_array, cw_dir_array, num_bytes);
    }

  // malloc plenty of space for token array
  char **tok_array = malloc(strlen(pathname) * sizeof(char*));
  char *lasts;
  char *tok = strtok_r(pathname, "/", &lasts);
  int tok_count = 0;

  // save all the tokens in the token array for use
  while (tok != NULL)
    {
    tok_array[tok_count++] = tok;
    tok = strtok_r(NULL, "/", &lasts);
    }

  // iterate through the token array and check if the directory exists. if it
  // exists, read it into memory and repeat the check until we reach the n-1 token.
  for (int i = 0; i < tok_count - 1; i++)
    {
    // check if current token is found in the current directory array
    int found = get_de_index(tok_array[i], dir_array);

    // if it is not found, or if it is not a directory, return NULL
    if (found == -1 || dir_array[found].attr != 'd') 
      {
      free(pathname);
      pathname = NULL;
      free(dir_array);
      dir_array = NULL;
      free(tok_array);
      tok_array = NULL;
      return NULL;
      }

    // read the containing directory array before finishing or iterating to the
    // next token
    LBAread(dir_array, num_blocks, dir_array[found].loc);
    }
  
  free(pathname);
  pathname = NULL;
  free(tok_array);
  tok_array = NULL;

  return dir_array;
  }

// interface to get the current working directory
// return 0 if successful, -1 otherwise
char* fs_getcwd(char *pathname, size_t size)
  {
  // we are saving the current working directory path as a global variable, so the
  // arguments are ignored and the global variable is returned
  return cw_path;
  }
  
// interface to set the current working directory:
int fs_setcwd(char *pathname)
  {
  // if the pathname is the root directory, load the root directory and set the
  // current working directory path
  if (strcmp(pathname, "/") == 0)
    {
    if (LBAread(cw_dir_array, fs_vcb->root_blocks, fs_vcb->root_loc) != fs_vcb->root_blocks)
      {
      perror("LBAread failed when trying to read the directory\n");
      }

    set_cw_path();
    
    return 0;
    }

  // load the directory of the path
  DE *dir_array = parsePath(pathname);

  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", pathname);
    return -1;
    }

  // save the last entry in the path
  char *last_tok = get_last_tok(pathname);

  // get the index of the last entry in the path.
  int found = get_de_index(last_tok, dir_array);

  // if not found, or not a directory
  if (found == -1 || dir_array[found].attr != 'd')
    {
    printf("fs_setcwd: cannot change directory to '%s': No such file or directory\n", pathname);
    return -1;
    }

  // read the directory into the current working directory array and the current
  // working directory path
  if (LBAread(cw_dir_array, dir_array[found].num_blocks, 
    dir_array[found].loc) != dir_array[found].num_blocks)
    {
    perror("LBAread failed when trying to read the directory\n");
    }

  set_cw_path();

  return 0;
  }

// make directory interface:
// returns the LBA block location of the directory array where the new
// directory is created, otherwise -1 if mkdir fails.
int fs_mkdir(const char *pathname, mode_t mode)
  {
  // save pathname to a mutable local variable
  char *path = malloc(strlen(pathname) + 1);
  strcpy(path, pathname);

  // load the directory that the new directory is supposed to be located in
  DE *dir_array = parsePath(path);

  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", path);
    return -1;
    }

  // get last token of the path
  char *last_tok = get_last_tok(path);

  // look for a directory entry that matches the name
  int found = get_de_index(last_tok, dir_array);

  // if an index is found, a file/directory already exists
  if (found > -1)
    {
    printf("fs_mkdir: cannot create directory ‘%s’: file or directory exists\n", path);
    return -1;
    }

  // find an available directory entry location in the directory
  int new_dir_index = get_avail_de_idx(dir_array);

  // no space left
  if (new_dir_index == -1)
    {
    return -1;
    }

  // initialize a new directory with this directory being the parent
  int new_dir_loc = init_dir(dir_array[0].loc);
  
  // calculate the number of blocks and bytes this directory occupies
  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;

  // New directory entry initialization
  dir_array[new_dir_index].size = num_bytes;
  dir_array[new_dir_index].num_blocks = num_blocks;
  dir_array[new_dir_index].loc = new_dir_loc;
  time_t curr_time = time(NULL);
  dir_array[new_dir_index].created = curr_time;
  dir_array[new_dir_index].modified = curr_time;
  dir_array[new_dir_index].accessed = curr_time;
  dir_array[new_dir_index].attr = 'd';
  strcpy(dir_array[new_dir_index].name, last_tok);

  // print_de(&dir_array[new_dir_index]);

  // write new directory to file system
  write_all_fs(dir_array);

  // the following are malloc'd in functions or explicitly here
  // functions that malloc: parsePath, get_last_tok
  free(dir_array);
  dir_array = NULL;
  free(last_tok);
  last_tok = NULL;
  free(path);
  path = NULL;

  return new_dir_loc;
  };

// remove directory interface:
// returns 0 if successful, -1, otherwise.
int fs_rmdir(const char *pathname)
  {
  // malloc mutable pathname
  char *path = malloc(strlen(pathname) + 1);
  strcpy(path, pathname);

  // load directory
  DE *dir_array = parsePath(path);

  // get final token from path
  char *last_tok = get_last_tok(path);

  // get index of final token in directory array
  int found_index = get_de_index(last_tok, dir_array);

  // must exist and be a directory
  if (found_index < 2 || dir_array[found_index].attr != 'd')
    {
    free(path);
    path = NULL;
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    perror("fs_rmdir: remove directory failed: Not a directory");
    return -1;
    }

  // check if the directory is empty
  if (!is_empty(&dir_array[found_index]))
    {
    free(path);
    path = NULL;
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    perror("fs_rmdir: remove directory failed: Directory not empty");
    return -1;
    }

  // set to available and reset the name, size, and num_blocks
  dir_array[found_index].name[0] = '\0';
  dir_array[found_index].num_blocks = 0;
  dir_array[found_index].size = 0;
  dir_array[found_index].attr = 'a';

  // restore the freespace that this directory occupies
  restore_free(&dir_array[found_index]);

  // write all changes to the file system to disk
  write_all_fs(dir_array);

  free(path);
  path = NULL;
  free(dir_array);
  dir_array = NULL;
  free(last_tok);
  last_tok = NULL;

  return 0;
  }

// delete file interface
int fs_delete(char *filename)
  {
  // malloc mutable path string
  char *path = malloc(strlen(filename) + 1);
  strcpy(path, filename);

  // load directory
  DE *dir_array = parsePath(path);

  // get last token in path
  char *last_tok = get_last_tok(path);

  // look for filename/token
  int found_index = get_de_index(last_tok, dir_array);

  // must exist and be a file
  if (found_index < 2 || dir_array[found_index].attr != 'f')
    {
    free(path);
    path = NULL;
    free(dir_array);
    dir_array = NULL;
    free(last_tok);
    last_tok = NULL;
    perror("fs_delete: Delete file failed: Not a file");
    return -1;
    }

  // set to available and reset the name, size, and num_blocks
  dir_array[found_index].name[0] = '\0';
  dir_array[found_index].num_blocks = 0;
  dir_array[found_index].size = 0;
  dir_array[found_index].attr = 'a';

  // restore the freespace that this file occupies
  restore_free(&dir_array[found_index]);

  // write all changes to the file system to disk
  write_all_fs(dir_array);

  free(path);
  path = NULL;
  free(dir_array);
  dir_array = NULL;
  free(last_tok);
  last_tok = NULL;

  return 0;
  }

// isFile interface
int fs_isFile(char *filename)
  {
  // load directory
  DE *dir_array = parsePath(filename);
  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", filename);
    return 0;
    }

  // get the index of the filename
  int index_found = get_de_index(get_last_tok(filename), dir_array);
  
  // must exist and be a file
  if (index_found < 2 || dir_array[index_found].attr != 'f')
    {
    perror("File/directory not found\n");
    return 0;
    }

  return 1;
  }

// isDir interface
int fs_isDir(char *pathname)
  {
  DE *dir_array = parsePath(pathname);
  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", pathname);
    return 0;
    }

  int index_found = get_de_index(get_last_tok(pathname), dir_array);
  
  // must exist and be a directory
  if (index_found < 0 || dir_array[index_found].attr != 'd')
    {
    perror("File/directory not found\n");
    return 0;
    }

  return 1;
  }

// fills fs_stat buffer with data from the path provided
// returns the index of the directory array if successful
// otherwise -1 if it fails
int fs_stat(const char *path, struct fs_stat *buf)
  {
  // malloc mutable path string
  char *pathname = malloc(strlen(path) + 1);
  strcpy(pathname, path);

  // load directory
  DE *dir_array = parsePath(pathname);
  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", pathname);
    return -1;
    }

  // retrieve directory entry index
  int index_found = get_de_index(get_last_tok(pathname), dir_array);
  if (index_found == -1)
    {
    perror("File/directory not found\n");
    return -1;
    }
  
  // fill the fs_stat buffer with the proper information
  buf->st_size = dir_array[index_found].size;
  buf->st_blksize = fs_vcb->block_size;
  buf->st_blocks = dir_array[index_found].num_blocks;
  buf->st_accesstime = dir_array[index_found].accessed;
  buf->st_modtime = dir_array[index_found].modified;
  buf->st_createtime = dir_array[index_found].created;

  // parsePath mallocs dir_array so needs to be freed
  free(dir_array);
  dir_array = NULL;

  return index_found;
  }

// open directory interface
fdDir * fs_opendir(const char *pathname)
  {
  // malloc mutable path string
  char *path = malloc(strlen(pathname) + 1);
  strcpy(path, pathname);

  // load directory
  DE *dir_array = parsePath(path);

  // retrieve file/directory name
  char *last_tok = get_last_tok(path);
  
  // retrieve index of directory entry
  int found = get_de_index(last_tok, dir_array);

  // must exist and be a directory
  if (found < 0 || dir_array[found].attr != 'd')
    {
    printf("fs_opendir: open directory failed: %s not found", last_tok);
    return NULL;
    }

  // malloc descriptor for directory
  fdDir *fd_dir = malloc(sizeof(fdDir));

  // fill the information in from the found directory
  fd_dir->d_reclen = dir_array[found].num_blocks;
  fd_dir->dirEntryPosition = found;
  fd_dir->directoryStartLocation = dir_array[found].loc;
  fd_dir->cur_item_index = 0;
  
  struct fs_diriteminfo *diriteminfo = malloc(sizeof(struct fs_diriteminfo));
  fd_dir->diriteminfo = diriteminfo;
  strcpy(fd_dir->diriteminfo->d_name, last_tok);
  fd_dir->diriteminfo->d_reclen = dir_array[found].num_blocks;
  fd_dir->diriteminfo->fileType = dir_array[found].attr;

  free(path);
  path = NULL;
  free(dir_array);
  dir_array = NULL;
  free(last_tok);
  last_tok = NULL;
  
  return fd_dir;
  }

// read directory interface
// this function returns the current directory item information. subsequent
// calls retrieves the next directory item.
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
  {
  if (dirp == NULL)
    {
    perror("fs_readdir: read directory failed: fdDir is NULL\n");
    return NULL;
    }

  // malloc directory array and load the directory from disk into memory
  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;
  DE* dir_array = malloc(num_bytes);
  LBAread(dir_array, num_blocks, dirp->directoryStartLocation);

  // increment the current item index if the item is available/empty (e.g. move
  // the index to the next occupied index)
  while (dir_array[dirp->cur_item_index].attr == 'a' && dirp->cur_item_index < DE_COUNT - 1)
    {
    dirp->cur_item_index++;
    }

  // return NULL if not found before the end of the directory array
  if (dirp->cur_item_index == DE_COUNT - 1)
    {
    free(dir_array);
    dir_array = NULL;
    return NULL;
    }

  // fill in the information for the directory item info as appropriate
  strcpy(dirp->diriteminfo->d_name, dir_array[dirp->cur_item_index].name);
  dirp->diriteminfo->d_reclen = dirp->d_reclen;
  dirp->diriteminfo->fileType = dir_array[dirp->cur_item_index].attr;

  dirp->cur_item_index++;

  free(dir_array);
  dir_array = NULL;

  return dirp->diriteminfo;
  }

//fs_closedir close the directory of the file system
int fs_closedir(fdDir *dirp)
  {
  if (dirp == NULL)
    {
    perror("fs_closedir: close directory failed: fdDir is NULL\n");
    return 0;
    }

  free(dirp->diriteminfo);
  dirp->diriteminfo = NULL;
  free(dirp);
  dirp = NULL;

  return 1;

  // following code contributed by Chengkai Yang
  // DIR *dir;
  // struct dirent *entry;
  // //prinf("The directory has already closed\n");

  // //checkout the pathname is NUll, perror
  // if ((dir = opendir("/")) == NULL){
  // perror("opendir() error");
  // }
  // while ((entry = fs_readdir(dir)) !=NULL){
  // //need some help buildup what is going to print out
  // //printf("xxxxxx")
  // }
  // closedir(dir);
  }

// checks if directory is empty
int is_empty(DE *d_entry)
  {
  if (d_entry == NULL)
    {
    perror("is_empty: failed: directory entry is NULL\n");
    return -1;
    }
  
  if (d_entry->attr != 'd')
    {
    perror("is_empty: failed: not a directory\n");
    return -1;
    }

  // malloc a directory entry array
  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;
  DE* dir_array = malloc(num_bytes);  // malloc space for a directory

  // load the directory entry array into memory
  if (LBAread(dir_array, d_entry->num_blocks, d_entry->loc) != d_entry->num_blocks)
    {
    perror("is_empty: LBAread failed\n");
    return -1;
    }

  // iterate through the directory to see if any of the entries is not available
  // (besides the current and parent entries)
  int is_empty = 1;
  for (int i = 2; i < DE_COUNT; i++)
    {
    if (dir_array[i].attr != 'a')
      {
      is_empty = 0;
      break;
      }
    }
  
  return is_empty;
  }

// helper function to retrieve the index of a named token in a directory
// (e.g. directory entry array)
// returns the index if it finds a match or -1 otherwise if not found
int get_de_index(char *token, DE* dir_array)
  {
  for (int i = 0; i < DE_COUNT; i++)
    {
    if (strcmp(token, dir_array[i].name) == 0)
      {
      return i;
      }
    }
  return -1;
  }

// helper function to get the first available directory entry in a
// directory.  returns the index of the directory array upon success
// or -1 otherwise
int get_avail_de_idx(DE* dir_array)
  {
  for (int i = 2; i < DE_COUNT; i++)
    {
    if (dir_array[i].attr == 'a')
      {
      return i;
      }
    }
  perror("No available space in directory\n");
  return -1;
  }

// print the entire directory array for debug purposes
void print_dir(DE* dir_array)
  {
  char *name = malloc(128);
  int loc = dir_array[0].loc;
  if (loc == dir_array[1].loc)
    {
    strcpy(name, "root");
    }
  else
    {
    int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
    int num_bytes = num_blocks * fs_vcb->block_size;
    DE* name_dir_array = malloc(num_bytes);
    LBAread(name_dir_array, dir_array[1].num_blocks, dir_array[1].loc);
    for (int i = 2; i < DE_COUNT; i++)
      {
      if (name_dir_array[i].loc == loc)
        {
        strcpy(name, name_dir_array[i].name);
        break;
        }
      }
    free(name_dir_array);
    name_dir_array = NULL;
    }
  printf("=================== Printing Directory Map ===================\n");
  printf("Directory Location: %#010lx   Directory Name: %s\n\n", dir_array[0].loc*4 + 1024, name);
  printf("   idx  Size        Loc in HD   Blocks  Att");
  printf("   idx  Size        Loc in HD   Blocks  Att");
  printf("   idx  Size        Loc in HD   Blocks  Att\n");

  for (int i = 0; i < DE_COUNT; i++)
    {
    printf("    %2d  %#010lx  %#010lx  %#06x  %c  ", 
        i, dir_array[i].size, dir_array[i].loc*sizeof(int) + 0x0400, dir_array[i].num_blocks, dir_array[i].attr);
    if ((i + 1) % 3 == 0) printf("\n");
    }
  printf("\n");
  }

// print a directory entry for debug purposes
void print_de(DE* d_entry)
  {
  printf("=================== Printing Directory Entry ===================\n");
  printf("Size: %#010lx\nLocation: %#010lx\nCreated: %#010lx\nModified: %#010lx\n",
    d_entry->size, d_entry->loc*sizeof(int) + 0x0400, d_entry->created, d_entry->modified);
  printf("Accessed: %#010lx\nAttribute: %c\nName: %s\nNumber Of Blocks: %d\n",
    d_entry->accessed, d_entry->attr, d_entry->name, d_entry->num_blocks);
  }
