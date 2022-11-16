#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//Needs to add below this one to the fsDir for using opendir()
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
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
        //     int size;		// size of the file in bytes
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

//Editting from here by Chengkai Yang
//fs_opendir open the directory of the file

static int locate_avail_fd();
static int locate_file(const char*pathname);
struct file_descriptor_t fd_table[FS_OPEN_MAX_COUNT];
//file descriptor is one unique open file of the process.
struct file_descriptor_t {
    bool   is_used;       
    int    file_index;              
    size_t offset;  
    char path_name[FS_PATHNAME_LEN];
};


fdDir * fs_opendir(const char *pathname);
{
        //Finding the file using locate_file
        int path_index = locate_file(pathname);
        if(path_index == -1){
                perror("file @[%s] doesn't exist\n", pathname);
                return -1;
        }

        int fd = locate_avail_fd();
        //Return file descriptor index, or other wise -1 on failure
        if (fd == -1){
                perror("file descriptors already allocated\n");
                return -1;
        }
        fd_table[fd].is_used    = true;
	fd_table[fd].file_index = path_index;
	fd_table[fd].offset     = 0;
	
	strcpy(fd_table[fd].path_name, pathname); 

        return fd;

}

//fs_readdir read the directory of the file
struct fs_diriteminfo fsDi;
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
	DIR *dir;
	dir = (DIR *)dirp;
	struct dirent *de;
	de = readdir(dir);
	if(de == NULL){
		return (NULL);
	}
	fsDi.d_reclen = (unsigned short)sizeof(fs_opendir);
	fsDi.fileType = de->d_type;
	strcpy(fsDi.d_name,de->d_name);
	return (&fsDi);
}

//fs_closedir close the directory of the file system
int fs_closedir(fdDir *dirp)
{
        DIR *dir;
        struct dirent *entry;
        //prinf("The directory has already closed\n");

        //checkout the pathname is NUll, perror
        if ((dir = opendir("/")) == NULL){
        perror("opendir() error");
        }
        while ((entry = fs_readdir(dir)) !=NULL){
        //need some help buildup what is going to print out
        //printf("xxxxxx")
        }
	closedir(dir);
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

void print_de(DE *dir)
        {
        printf("=================== Printing Directory Entry ===================\n");
        printf("Size: %d\nLocation: %d\nCreated: %ld\nModified: %ld\nAccessed: %ld\nAttribute: %s\nName: %s\n",
                dir->size, dir->loc, dir->created, dir->modified, dir->accessed, dir->attr, dir->name);
        }


