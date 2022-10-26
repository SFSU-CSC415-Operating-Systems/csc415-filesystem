#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fsDir.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsFree.h"
#include "fsHelpers.h"

#define DE_COUNT 40		// initial number of d_entries to allocate to a directory

// Initialize a directory
// If parent_loc == 0, this is the root directory
int init_dir(VCB *fs_vcb, int *freespace, int parent_loc)
        {
        printf("***** init_dir *****\n");

        int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
        printf("Number of blocks for the root dir: %d\n", num_blocks);
        int num_bytes = num_blocks * fs_vcb->block_size;
        printf("Number of bytes for the root dir: %d\n", num_bytes);
        printf("Size of DE: %lu\n", sizeof(DE));
        DE* dir_array = malloc(num_bytes);
        int dir_loc = alloc_free(fs_vcb, freespace, num_blocks);
        printf("Directory Location: %d\n", dir_loc);

        if (dir_loc == -1)
                {
                perror("Allocation of directory failed.\n");
                free(dir_array);
                dir_array = NULL;
                return -1;
                }

        // typedef struct
        //     {
        //     int size;			// size of the file in bytes
        //     int loc;			// block location of file
        //     time_t created;		// time file was created
        //     time_t modified;	        // time file was last modified
        //     time_t accessed;	        // time file was last accessed
        //     char attr;		// attributes of file ('d': directory, 'f': file)
        //     char name[43];		// name of file
        //     } DE;

        // Directory "." entry initialization
        dir_array[0].size = num_bytes;
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
                dir_array[1].loc = dir_loc;        // currently the only difference
                dir_array[1].created = curr_time;
                dir_array[1].modified = curr_time;
                dir_array[1].accessed = curr_time;
                }
        else
                {
                // How do I pull directory info from the parent?
                // Would this even be necessary?
                dir_array[1].size = num_bytes;
                dir_array[1].loc = parent_loc;     // currently the only difference
                dir_array[1].created = curr_time;
                dir_array[1].modified = curr_time;
                dir_array[1].accessed = curr_time;
                }
        strcpy(dir_array[1].attr, "d");
        strcpy(dir_array[1].name, "..");

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