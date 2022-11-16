#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fsDir.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsFree.h"
#include "fsHelpers.h"
#include "fsDir.h"

// Initialize a directory
// If parent_loc == 0, this is the root directory
int init_dir(int parent_loc)
  {
  printf("***** init_dir *****\n");

  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  printf("Number of blocks in dir: %d\n", num_blocks);
  int num_bytes = num_blocks * fs_vcb->block_size;
  printf("Number of bytes in dir: %d\n", num_bytes);
  printf("Size of DE: %lu\n", sizeof(DE));
  DE* dir_array = malloc(num_bytes);
  int dir_loc = alloc_free(num_blocks);
  printf("Directory Location: %d\n", dir_loc);

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
  strcpy(dir_array[0].attr, "d");
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
    // Need to LBAread the parent to get all the data about the parent
    // Is this even necessary?
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
  strcpy(dir_array[1].attr, "d");
  strcpy(dir_array[1].name, "..");

  for (int i = 2; i < DE_COUNT; i++)
    {
    // set all other directory entries to available
    strcpy(dir_array[i].attr, "a");
    }

  // print_de(&dir_array[0]);
  // print_de(&dir_array[1]);

  int blocks_written = LBAwrite(dir_array, num_blocks, dir_loc);

  if (blocks_written != num_blocks)
    {
    perror("LBAwrite failed\n");
    return -1;
    }

  printf("LBAwrite blocks written: %d\n", blocks_written);
  
  free(dir_array);
  dir_array = NULL;

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
  printf("***** parsePath *****\n");
  char *pathname = malloc(strlen(path) + 1);
  strcpy(pathname, path);

  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;
  DE* dir_array = malloc(num_bytes);  // malloc space for a directory

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

  while (tok != NULL)
    {
    tok_array[tok_count++] = tok;
    tok = strtok_r(NULL, "/", &lasts);
    }

  for (int i = 0; i < tok_count - 1; i++)
    {

    int found = get_de_index(tok_array[i], dir_array);

    if (found == -1) 
      {
      free(dir_array);
      dir_array = NULL;
      free(tok_array);
      tok_array = NULL;
      return NULL;
      }

    LBAread(dir_array, num_blocks, dir_array[found].loc);
    }
  
  free(tok_array);
  tok_array = NULL;

  return dir_array;
  }

char* fs_getcwd(char *pathname, size_t size)
  {
  return cw_path;
  }
  
int fs_setcwd(char *pathname)
  {
  DE *dir_array = parsePath(pathname);

  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", pathname);
    return -1;
    }

  // printf("Old path: '%s'\n", pathname);
  char *last_tok = get_last_tok(pathname);
  // printf("New path: '%s'\n", pathname);

  int found = get_de_index(last_tok, dir_array);

  if (found == -1)
    {
    printf("fs_setcwd: cannot change directory to '%s': No such file or directory\n", pathname);
    return -1;
    }

  if (LBAread(cw_dir_array, dir_array[found].num_blocks, dir_array[found].loc) != dir_array[found].num_blocks)
    {
    perror("LBAread failed when trying to read the directory\n");
    }

  set_cw_path();

  return 0;
  }

// This function makes a new directory at the pathname given
// returns the LBA block location of the directory array where the new
// directory is created, otherwise -1 if mkdir fails.
int fs_mkdir(const char *pathname, mode_t mode)
  {
  printf("****** fs_mkdir ******\n");
  char *path = malloc(strlen(pathname) + 1);
  strcpy(path, pathname);
  printf("Path: %s\n", path);

  DE *dir_array = parsePath(path);

  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", path);
    return -1;
    }

  char *last_tok = get_last_tok(path);
  
  printf("Last Token: '%s'\n", last_tok);

  int found = get_de_index(last_tok, dir_array);
  printf("Found DE index: %d\n", found);

  if (found > -1)
    {
    printf("fs_mkdir: cannot create directory ‘%s’: No such file or directory\n", path);
    return -1;
    }

  int new_dir_index = get_avail_de_idx(dir_array);
  printf("New directory index: %d\n", new_dir_index);
  if (new_dir_index == -1)
    {
    return -1;
    }

  int new_dir_loc = init_dir(dir_array[0].loc);
  printf("New directory LBA loc: %d\n", new_dir_loc);
  
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
  strcpy(dir_array[new_dir_index].attr, "d");
  strcpy(dir_array[new_dir_index].name, last_tok);

  // write all changes to VCB, freespace, and directory to disk
  if (LBAwrite(fs_vcb, 1, 0) != 1)
		{
		perror("LBAwrite failed when trying to write the VCB\n");
		}
	
	if (LBAwrite(freespace, fs_vcb->freespace_size, 1) != fs_vcb->freespace_size)
		{
		perror("LBAwrite failed when trying to write the freespace\n");
		}
	
	if (LBAwrite(dir_array, dir_array[0].num_blocks, dir_array[0].loc) != dir_array[0].num_blocks)
		{
		perror("LBAwrite failed when trying to write the directory\n");
		}

  // if the current directory is where the directory was created
  // update the current directory
  if (dir_array[0].loc == cw_dir_array[0].loc)
    {
    memcpy(cw_dir_array, dir_array, dir_array[0].size);
    }

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


// fills fs_stat buffer with data from the path provided
// returns the index of the directory array if successful
// otherwise -1 if it fails
int fs_stat(const char *path, struct fs_stat *buf)
  {
  char *pathname = malloc(strlen(path) + 1);
  strcpy(pathname, path);

  DE *dir_array = parsePath(pathname);
  if (dir_array == NULL)
    {
    printf("Invalid path: %s\n", pathname);
    return -1;
    }

  int index_found = get_de_index(get_last_tok(pathname), dir_array);
  
  if (index_found == -1)
    {
    perror("File/directory not found\n");
    return -1;
    }
  
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

// sets the current working path
char* set_cw_path()
  {
  int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
  int num_bytes = num_blocks * fs_vcb->block_size;
  DE *dir_array = malloc(num_bytes);

  char **tok_array = malloc(MAX_PATH_LENGTH);
  int tok_count = 0;

  LBAread(dir_array, cw_dir_array[0].num_blocks, cw_dir_array[0].loc);

  if (dir_array[0].loc == dir_array[1].loc)
    {
    strcpy(cw_path, "/");
    return cw_path;
    }

  int search_loc = dir_array[0].loc;
  int path_size = 0;
  while (dir_array[0].loc != dir_array[1].loc)
    {
    LBAread(dir_array, dir_array[1].num_blocks, dir_array[1].loc);
    for (int i = 2; i < DE_COUNT; i++)
      {
        if (dir_array[i].loc == search_loc)
          {
          int size_of_name = strlen(dir_array[i].name) + 1;
          path_size += size_of_name;
          tok_array[tok_count] = malloc(size_of_name);
          strcpy(tok_array[tok_count++], dir_array[i].name);
          break;
          }
      }
    search_loc = dir_array[0].loc;
    }

  char *path = malloc(path_size);
  strcpy(path, "/");
  for (int i = tok_count - 1; i >= 0; i--)
    {
    strcat(path, tok_array[i]);

    free(tok_array[i]);
    tok_array[i] = NULL;

    if (i > 0)
      strcat(path, "/");
    }

  strcpy(cw_path, path);

  free(tok_array[0]);
  tok_array[0] = NULL;
  free(dir_array);
  dir_array = NULL;
  free(tok_array);
  tok_array = NULL;

  return path;
  }

// helper function to get the last token/file/directory name from a path
char* get_last_tok(const char *pathname)
  {
  char *path = malloc(strlen(pathname) + 1);
  strcpy(path, pathname);
  char *lasts;
  char *tok = strtok_r(path, "/", &lasts);
  char *ret = malloc(sizeof(char) * 128);

  while (tok != NULL)
    {
    strcpy(ret, tok);
    tok = strtok_r(NULL, "/", &lasts);
    }

  return ret;
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
    if (strcmp("a", dir_array[i].attr) == 0)
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
  printf("Directory Location: %li   Directory Name: %s\n\n", dir_array[0].loc, name);
  printf("   idx  Size        Loc         Blocks  Att   idx  Size        Loc         Blocks  Att   idx  Size        Loc         Blocks  Att\n");

  for (int i = 0; i < DE_COUNT; i++)
    {
    printf("    %2d  %#010lx  %#010lx  %#06x  %s  ", 
        i, dir_array[i].size, dir_array[i].loc, dir_array[i].num_blocks, dir_array[i].attr);
    if ((i + 1) % 3 == 0) printf("\n");
    }
  printf("\n");
  }

// print a directory entry for debug purposes
void print_de(DE* dir)
  {
  printf("=================== Printing Directory Entry ===================\n");
  printf("Size: %lu\nLocation: %li\nCreated: %ld\nModified: %ld\nAccessed: %ld\nAttribute: %s\nName: %s\n",
          dir->size, dir->loc, dir->created, dir->modified, dir->accessed, dir->attr, dir->name);
  }
