#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define align4(x) ((x - 1) / 4 + 1) * 4

unsigned char *disk;
char dir[1024+1];
char* inodebitmap;
char* blockbitmap;
struct ext2_inode* inode_table;

//make them global

int find_inode_index(char* dir, int i,struct ext2_inode* inode_table,int flag,int path_flag);
//void bitmap_reset(char* bitmap, int index, int value);


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
int get_path_inode(char path[],int inode_idx,struct ext2_inode* inode_table,int path_flag){
     char new_path[strlen(path)+1];
     memcpy(new_path, path, strlen(path));
     new_path[ strlen(path)] = 0;
     int pos;
     int check;
     
     
     while ((pos = next_path(new_path) ) > 1){
        
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
         // find the postion
        
        if (strlen(new_path) < 2){
            inode_idx = find_inode_index(dir, inode_idx,inode_table,1,path_flag); // check file exists
           

            return inode_idx;
            
        }else{
        //printf("%s\n",new_path);
        //printf("%s\n",dir);
        //call find_inode_index to find the inode
            inode_idx = find_inode_index(dir, inode_idx,inode_table,2,path_flag); // 2 for find a directory
            

            if (inode_idx == -1){
                return -1;
            }
        }
       
        //printf("find:%d\n",inode_idx);
        
    
    }
    return inode_idx;
    
    
}

int find_inode_index(char* dir, int inode_num, struct ext2_inode* inode_table,int flag, int path_flag){
    
    struct ext2_inode target_inode = inode_table[inode_num-1];
    int num_block = target_inode.i_size / EXT2_BLOCK_SIZE;
    struct ext2_dir_entry_2* entry;
    int block_num;
    int idx;
    //int return_value = -1;
    for (idx=0; idx< num_block; idx++){
                block_num = target_inode.i_block[idx];
                entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * block_num); 

                int size = 0;
                
               
                
                while (size < EXT2_BLOCK_SIZE){
                       size += entry->rec_len;
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

                        if(entry->file_type == flag){
                        char file_name[255+1];
                        memcpy(file_name, entry->name, entry->name_len);
                        file_name[entry->name_len] = 0;
                        
                            if (strncmp(file_name, dir, entry->name_len) == 0 && strlen(dir) == entry->name_len){
                                
                                    if (path_flag == 2){
                                        if (flag == 2) return entry->inode;
                                        else if (flag == 1) return -2;
                                    }
                                    else if (path_flag == 1){
                                        return entry->inode;
                                        

                                    }
                                }
                                //return entry->inode;
                        }
                        entry = (void *)entry + entry->rec_len;

                      }
  
    }
    if (path_flag == 2){
        if (flag == 2) return -1;
        else if (flag == 1) return inode_num;
    }

   return -1;
}



int find_free (struct ext2_super_block *sb, struct ext2_group_desc *gd, int flag){ //flag 1 for block, 2 for inode
    char *bitmap;
    int i;
    int j;
    int count;
    if (flag == 1){
        if (sb->s_free_blocks_count <= 0) return 0;
        bitmap = (char *)(disk + (gd->bg_block_bitmap * EXT2_BLOCK_SIZE));
        i = 0;
        j = 0;
        count = sb->s_free_blocks_count;

    }
    else if (flag == 2){
        if (sb->s_free_inodes_count <= 0) return 0;
        bitmap = (char *)(disk + (gd->bg_inode_bitmap * EXT2_BLOCK_SIZE));
        i = 1;
        j =  (EXT2_GOOD_OLD_FIRST_INO - 1) % 8;
        count = sb->s_free_inodes_count;
    }

    while ( i  <  count / 8){
        while (j < 8){
            if (((bitmap[i] >> j) & 1) == 0){
                break;
            }
            j++;
        }
        if (j < 8) {break;}
        j = 0;
        i++;
    }
    return (i*8)+j+1;
}

void fill_bitmap(int num, struct ext2_super_block *sb, struct ext2_group_desc *gd, int flag){ //flag 1 for block, 2 for inode
    char * bitmap;
    if (flag == 1){
        sb->s_free_blocks_count -= 1;
        bitmap = (char *)(disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE);
    }
    else if (flag == 2){
        sb->s_free_inodes_count -= 1;
        bitmap = (char *)(disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE);

    }
    char byte;
    byte = bitmap[(num - 1) / 8];
    int pos = (num - 1) % 8;

    bitmap[(num - 1) / 8] = byte | (1 << pos);
}


int check_free_block(struct ext2_super_block *sb, struct ext2_group_desc *gd){
    int free_block = find_free(sb,gd,1);
    if (free_block == 0){
        errno = ENOSPC;
        perror("No free inode");
        exit(errno);
    }
    return free_block;

}






int main(int argc, char **argv) {

    if(argc != 4) {
        fprintf(stderr, "Usage: ext2_ln <image file name> <file location in image> <file stored in image>\n");
        exit(1);
    }

    

    //get absolute path
    int path_len1 = strlen(argv[2]);
    char path_name1[path_len1+1];
    memcpy(path_name1, argv[2], path_len1);
    path_name1[path_len1] = 0;
    
    //check whether the path is valid
    if (path_name1[0] != '/'){
        fprintf(stderr, "Please provide valid absolute path\n");
        exit(1);
    }
   
    
    
    //get absolute path
    int path_len2 = strlen(argv[3]);
    char path_name2[path_len2+1];
    memcpy(path_name2, argv[3], path_len2);
    path_name2[path_len2] = 0;
    
    //check whether the path is valid
    if (path_name2[0] != '/'){
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

    
    blockbitmap = (char*)(disk + (gd->bg_block_bitmap)* EXT2_BLOCK_SIZE);
    inodebitmap = (char*)(disk + (gd->bg_inode_bitmap)* EXT2_BLOCK_SIZE);
    inode_table = (struct ext2_inode*)(disk + (gd->bg_inode_table * EXT2_BLOCK_SIZE));

    
  
    //char* path_list[255+1];

    


    int inode_num1 = get_path_inode(path_name1, EXT2_ROOT_INO, inode_table,1);
    int dir1_len = strlen(dir);
    char *dir1 = malloc(sizeof(char)*dir1_len);
    strncpy(dir1,dir,dir1_len);
    printf("%d\n",inode_num1);
    printf("%s\n",dir1);


    int inode_num2 = get_path_inode(path_name2, EXT2_ROOT_INO, inode_table,2);
    //int inode_num2 = get_path_inode(path_name, EXT2_ROOT_INO, inode_table,2);

    printf("%d\n",inode_num2);
    printf("%s\n",dir);

    if (inode_num1 == -1){
        //printf("rm: %s: No such file or directory\n",dir);
        errno = ENOENT;
        perror(argv[2]);
        exit(errno);

    }
    //int check = -1;

    if (inode_num2 == -1){
        //printf("rm: %s: No such file or directory\n",dir);
        errno = ENOENT;
        perror(argv[3]);
        exit(errno);

    }
     
    
    
    if (inode_num2 == -2){
        errno = EEXIST;
        perror(dir);
        exit(errno);

    }

    //TODO
     int enough = 0;
      //dir has enough space for the name for new dir
    int new_dir_len = align4(strlen(dir)) + 8;
    struct ext2_inode parent_inode = inode_table[inode_num2 - 1];
    parent_inode.i_links_count ++;
    //printf("parent_inode:%d\n", inode_num);
    int i;
    struct ext2_dir_entry_2* curr_entry;
    for(i = 0; i < 16 && parent_inode.i_block[i]; i++){
        int curr_block = parent_inode.i_block[i];
        //printf("curr_block:%d\n", curr_block);
        curr_entry = (struct ext2_dir_entry_2*)( disk + EXT2_BLOCK_SIZE * curr_block);
        int curr_size = 0;
        while (curr_size < EXT2_BLOCK_SIZE){
            curr_size += curr_entry->rec_len;
            int need_len = (align4(curr_entry->name_len)+ 8 + new_dir_len);
            if (curr_size == EXT2_BLOCK_SIZE && curr_entry->rec_len >= need_len){
                enough = 1;
                break;
            }
            curr_entry = (void*)curr_entry + curr_entry->rec_len;
        }

    }

    if (enough == 1){
        int last_rec_len = curr_entry->rec_len;
        int actual_len = align4(curr_entry->name_len) + 8;
        curr_entry->rec_len = actual_len;
        //change to new entry
        curr_entry = (void*)curr_entry + actual_len;
        
        
        curr_entry->rec_len = last_rec_len - actual_len;
        


     }
     else if (enough == 0){ //not enough, need a new block 
        int new_free_block = find_free(sb,gd,1);
        if (new_free_block == 0){
        errno = ENOSPC;
        perror("No free block");
        exit(errno);
        }
        fill_bitmap(new_free_block, sb, gd, 1);
        curr_entry = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE * new_free_block);
        curr_entry->rec_len = EXT2_BLOCK_SIZE;
        inode_table[inode_num2 - 1].i_size += EXT2_BLOCK_SIZE;
        inode_table[inode_num2 - 1].i_blocks += 2;
        inode_table[inode_num2 - 1].i_block[i+1] = new_free_block;




     }

     curr_entry->inode = inode_num1;
     curr_entry->name_len = strlen(dir);
     curr_entry->file_type = EXT2_FT_REG_FILE; 
     strncpy(curr_entry->name, dir, strlen(dir));

    
    
   
   return 0;

}

