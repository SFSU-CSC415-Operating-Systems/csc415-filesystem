#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fsDir.h"
#include "fsLow.h"
#include "mfs.h"
#include "fsFree.h"
#include "fsHelpers.h"

#define DE_COUNT 64		// initial number of d_entries to allocate to a directory

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

        // typedef struct
        //     {
        //     int size;		// size of the file in bytes
        //     int loc;			// block location of file
        //     time_t created;		// time file was created
        //     time_t modified;	        // time file was last modified
        //     time_t accessed;	        // time file was last accessed
        //     char attr;		// attributes of file ('d': directory, 'f': file)
        //     char name[43];		// name of file
        //     } DE;

        // track number of blocks root directory occupies
        fs_vcb->root_blocks = num_blocks;

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

        for (int i = 2; i < DE_COUNT; i++)
                {
                // Directory "." entry initialization
                // dir_array[i].size = 0;
                // dir_array[i].loc = 0;
                // dir_array[i].created = 0;
                // dir_array[i].modified = 0;
                // dir_array[i].accessed = 0;
                strcpy(dir_array[i].attr, "a");
                // strcpy(dir_array[i].name, "");
                }

        print_de(&dir_array[0]);
        print_de(&dir_array[1]);

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

DE* parsePath(char *pathname)
        {
        printf("***** parsePath *****\n");

        DE* dir_array = alloc_dir_array();

        if (pathname[0] == '/')
                {
                LBAread(dir_array, fs_vcb->root_blocks, fs_vcb->root_loc);
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
                
                }

        return dir_array;
        }

DE* alloc_dir_array()
        {
        printf("******** alloc_dir_array **********");
        int num_blocks = get_num_blocks(sizeof(DE) * DE_COUNT, fs_vcb->block_size);
        printf("Number of blocks in dir: %d\n", num_blocks);
        int num_bytes = num_blocks * fs_vcb->block_size;
        printf("Number of bytes in dir: %d\n", num_bytes);
        printf("Size of DE: %lu\n", sizeof(DE));
        DE* dir_array = malloc(num_bytes);
        return dir_array;
        }

void print_dir(DE* dir_array)
        {
        printf("=================== Printing Directory Map ===================\n");
        printf("Directory Location: %d\n\nindex  Size    Loc     Att\n", dir_array[0].loc);
        for (int i = 0; i < 40; i++)
                {
                printf("%2d     %#06x  %#06x  %s\n", i, dir_array[i].size, dir_array[i].loc, dir_array[i].attr);
                }
        }

void print_de(DE* dir)
        {
        printf("=================== Printing Directory Entry ===================\n");
        printf("Size: %d\nLocation: %d\nCreated: %ld\nModified: %ld\nAccessed: %ld\nAttribute: %s\nName: %s\n",
                dir->size, dir->loc, dir->created, dir->modified, dir->accessed, dir->attr, dir->name);
        }