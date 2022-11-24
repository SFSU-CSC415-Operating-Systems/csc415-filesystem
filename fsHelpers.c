/**************************************************************
* Class:  CSC-415-02 Fall 2022
* Names: Mark Kim, Peter Truong, Chengkai Yang, Zeel Diyora
* Student IDs: 918204214, 915780793, 921572896, 920838201
* GitHub Name: mkim797
* Group Name: Diligence
* Project: Basic File System
*
* File: fsHelpers.c
*
* Description: Helper Operations
*
**************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fsHelpers.h"
#include "fsLow.h"
#include "b_io.h"

int get_num_blocks(int bytes, int block_size)
  {
  return (bytes + block_size - 1)/(block_size);
  }

void write_all_fs(DE *dir_array)
  {
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
  }

void write_dir(DE *dir_array)
  {
  // write changes to directory to disk
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

  if (tok == NULL)
    {
    strcpy(ret, ".");
    return ret;
    }

  while (tok != NULL)
    {
    strcpy(ret, tok);
    tok = strtok_r(NULL, "/", &lasts);
    }

  free(path);
  path = NULL;

  return ret;
  }