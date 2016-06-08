#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>

unsigned char *disk;

int find_inode_index(char* dir, int i,struct ext2_inode* inode_table);

#define MAX_PATH_DIR 100 /*the absolute path max have 100 dir*/

#define BASE_OFFSET 1024  /* location of the super-block in the first group */

#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*1024)


//helper function to track the absolute path

//find next "/"
int next_path(char path[]){
    int len = strlen(path);
    if (len ==1){
        return 0;
    }
    int i;
    for (i = 1;i < len; i++){
        if (path[i] ==  '/' ){
            break;
        }
    }
    return i;
}

//get the directory name and find the inode contains it
int get_path_inode(char path[],int inode_idx,struct ext2_inode* inode_table){
     char new_path[strlen(path)+1];
     memcpy(new_path, path, strlen(path));
     new_path[ strlen(path)] = 0;
     int pos;
     
     
     while ((pos = next_path(new_path) ) > 1){
        char dir[pos+1];
        int i;
        for (i = 0; i < pos - 1; i++){
            dir[i] = new_path[i+1];
        }
        dir[pos-1] = 0;

        
        int size = 0;
        
        char* new = malloc(sizeof(char)*strlen(path));
        new = (char*)(new_path + pos);
        memcpy(new_path, new, strlen(new));
        new_path[strlen(new)] = 0;
        //printf("%s\n",new_path);
        //printf("%s\n",dir);
        //call find_inode_index to find the inode
        inode_idx = find_inode_index(dir, inode_idx-1,inode_table);
        //printf("find:%d\n",inode_idx);
        if (inode_idx == -1){
            return -1; //directory not found
        }

    }
    return inode_idx;
}
int find_inode_index(char* dir, int i, struct ext2_inode* inode_table){
   
                int idx;
                for (idx = 0; idx < 16; idx++)
                {
                    int num = inode_table[i].i_block[idx];
                    if (num == 0){
                        break;
                    }
                    //printf("   DIR BLOCK NUM: %d (for inode %d)\n",num,i+1);
                
                    int size = 0;
                    int inode_size = inode_table[i].i_size;

                    struct ext2_dir_entry_2* entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * num);
                    while (size < inode_size){
                       //file type
                        //file_type   Description
                        //0   Unknown
                        //1   Regular File
                        //2   Directory
                        //3   Character Device
                        //4   Block Device
                        //5   Named pipe
                        //6   Socket
                        //7   Symbolic Link
                        if(entry->file_type == 2){
                        char file_name[255+1];
                        memcpy(file_name, entry->name, entry->name_len);
                        file_name[entry->name_len] = 0;
                        
                            if (strncmp(file_name, dir, entry->name_len) == 0 && strlen(dir) == entry->name_len){
                                return entry->inode;
                            }
                        }

                        size += entry->rec_len;
                        entry = (void *)entry + entry->rec_len;
               
                    }
            
                }
                return -1; //return -1 for not found
   
   }

void ls_print(struct ext2_inode* inode_table, int inode_num){
    int idx;
    for (idx = 0; idx < 16; idx++){
        int num = inode_table[inode_num].i_block[idx];
        if (num == 0){
                break;
        }
                   
        int size = 0;
        int inode_size = inode_table[inode_num].i_size;

        struct ext2_dir_entry_2* entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * num);
        while (size < inode_size){
                       
        
            char print_name[255+1];
            memcpy(print_name, entry->name, entry->name_len);
            print_name[entry->name_len] = 0;
            printf("%s\n",print_name);            
                        

            size += entry->rec_len;
            entry = (void *)entry + entry->rec_len;
               
        }
            
                
               
   
   }

}

int main(int argc, char **argv) {

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <directory>\n");
        exit(1);
    }
    
    
    
    //get absolute path
    int path_len = strlen(argv[2]);
    char path_name[path_len+1];
    memcpy(path_name, argv[2], path_len);
    path_name[path_len] = 0;
    
    //check whether the path is valid
    if (path_name[0] != '/'){
        fprintf(stderr, "Please provide valid absolute path\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
    
    //get the suprt block and group descripter
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    struct ext2_inode* inode_table = (struct ext2_inode*)(disk + (gd->bg_inode_table * EXT2_BLOCK_SIZE));
  
    //char* path_list[255+1];

    int inode_num = get_path_inode(path_name, EXT2_ROOT_INO, inode_table);
    //printf("%d\n",inode_num);
    if (inode_num == -1){
        printf("No such file or diretory");
        return -1;
    }

    ls_print(inode_table,inode_num-1);


   return 0;

}
