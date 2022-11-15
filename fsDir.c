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

  // track number of blocks root directory occupies
  fs_vcb->root_blocks = num_blocks;

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
          
  return dir_loc;
  }

// parsePath returns NULL if the path is invalid (e.g. one of the directories
// does not exist) or the n-1 directory (array of DE's) pointer
DE* parsePath(char *pathname)
  {
  printf("***** parsePath *****\n");

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
  tok_array[0] = tok;
  int tok_count = 1;

  while (tok != NULL)
    {
    tok = strtok_r(NULL, "/", &lasts);
    tok_array[tok_count++] = tok;
    }

  for (int i = 0; i < tok_count - 1; i++)
    {
    int found = 0;
    for (int j = 0; j < DE_COUNT; j++)
      {
      if (strcmp(tok_array[i], dir_array[j].name) == 0)
        {
        found = 1;
        LBAread(dir_array, num_blocks, dir_array[j].loc);
        break;
        }
      }
    if (!found) 
      {
      return NULL;
      }
    }

  return dir_array;
  }

int fs_mkdir(const char *pathname, mode_t mode)
  {
  
  };

int fs_stat(const char *path, struct fs_stat *buf)
  {
  DE *dir_array = parsePath(path);
  if (dir_array == NULL)
    {
    return -1;
    }
  return 0;
  }

void print_dir(DE* dir_array)
  {
  printf("=================== Printing Directory Map ===================\n");
  printf("Directory Location: %li   Directory Name: %s\n\n"), dir_array[0].loc, dir_array[0].name;
  printf("   idx  Size        Loc         Blocks  Att   idx  Size        Loc         Blocks  Att   idx  Size        Loc         Blocks  Att\n");

  for (int i = 0; i < DE_COUNT; i++)
    {
    printf("    %2d  %#010lx  %#010lx  %#06x  %s  ", 
        i, dir_array[i].size, dir_array[i].loc, dir_array[i].num_blocks, dir_array[i].attr);
    if ((i + 1) % 3 == 0) printf("\n");
    }
  printf("\n");
  }

void print_de(DE* dir)
  {
  printf("=================== Printing Directory Entry ===================\n");
  printf("Size: %lu\nLocation: %li\nCreated: %ld\nModified: %ld\nAccessed: %ld\nAttribute: %s\nName: %s\n",
          dir->size, dir->loc, dir->created, dir->modified, dir->accessed, dir->attr, dir->name);
  }
