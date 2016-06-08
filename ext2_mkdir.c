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

#define align4(x) ((x - 1) / 4 + 1) * 4

unsigned char *disk;
char dir[1024+1];
char* inodebitmap;
char* blockbitmap;
struct ext2_inode* inode_table;

//make them global

int find_inode_index(char* dir, int i,struct ext2_inode* inode_table,int flag);
//void bitmap_reset(char* bitmap, int index, int value);



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
            inode_idx = find_inode_index(dir, inode_idx,inode_table,2); // check exists
           

            return inode_idx;
            
        }else{
        //printf("%s\n",new_path);
        //printf("%s\n",dir);
        //call find_inode_index to find the inode
            inode_idx = find_inode_index(dir, inode_idx,inode_table,1); // 2 for find a directory
            

            if (inode_idx == -1){
                return -1;
            }
        }
       
        //printf("find:%d\n",inode_idx);
        
    
    }
    return inode_idx;
    
    
}

int find_inode_index(char* dir, int inode_num, struct ext2_inode* inode_table,int flag){
    int found = 0;
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

                        if(entry->file_type == 2){
                        char file_name[255+1];
                        memcpy(file_name, entry->name, entry->name_len);
                        file_name[entry->name_len] = 0;
                        
                            if (strncmp(file_name, dir, entry->name_len) == 0 && strlen(dir) == entry->name_len){
                                
                                    if (flag == 1) return entry->inode;
                                    else if (flag == 2) return -2;
                                }
                                //return entry->inode;
                        }
                        entry = (void *)entry + entry->rec_len;

                      }
  
     }
   if (flag == 1) return -1;
   else if (flag == 2) return inode_num;
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









int main(int argc, char **argv) {

    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_mkdir <image file name> <directory>\n");
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

    
    blockbitmap = (char*)(disk + (gd->bg_block_bitmap)* EXT2_BLOCK_SIZE);
    inodebitmap = (char*)(disk + (gd->bg_inode_bitmap)* EXT2_BLOCK_SIZE);
    inode_table = (struct ext2_inode*)(disk + (gd->bg_inode_table * EXT2_BLOCK_SIZE));

    
  
    //char* path_list[255+1];

    int inode_num = get_path_inode(path_name, EXT2_ROOT_INO, inode_table);

    //printf("%d\n",inode_num);
    //printf("%s\n",dir);

    if (inode_num == -1){
        //printf("rm: %s: No such file or directory\n",dir);
        errno = ENOENT;
        perror(dir);
        exit(errno);

    }
    //int check = -1;
     
    
    
    if (inode_num == -2){
        errno = EEXIST;
        perror(dir);
        exit(errno);

    }
    //find a empty inode and block, get their index
    int free_block = find_free(sb,gd,1);
    int free_inode = find_free(sb,gd,2);

    if (free_block == 0){
        errno = ENOSPC;
        perror("No free block");
        exit(errno);
    }
    if (free_inode == 0){
        errno = ENOSPC;
        perror("No free inode");
        exit(errno);
    }
    //printf("block:%d\n", free_block);
    //printf("inode:%d\n", free_inode);

    
    struct ext2_inode *new_inode = &(inode_table[free_inode - 1]);
   
    //fill the inform in the free inode
    fill_bitmap(free_inode, sb, gd, 2); // add 1 to proper postion of the inode_bitmap

    new_inode->i_links_count = 2; //empty dir
    new_inode->i_blocks = 2;
    new_inode->i_size = EXT2_BLOCK_SIZE;
    new_inode->i_mode = EXT2_S_IFDIR;
   
    new_inode->i_block[0] = free_block;
    //fill the i_block by 0
    int idx;
    for (idx = 1; idx < 15; idx ++){
        inode_table[free_inode - 1].i_block[idx] = 0;
    }


     //printf("test\n");
    //fill the inform in the new dir block
    struct ext2_dir_entry_2* n_dir = (struct ext2_dir_entry_2*)(disk + EXT2_BLOCK_SIZE*free_block);
    
    n_dir->inode = free_inode;
    n_dir->rec_len = 12;
    n_dir->name_len = 1;
    n_dir->file_type = EXT2_FT_DIR;//2
    strncpy(n_dir->name, ".", 1);

    n_dir = (void*) n_dir + 12;
    
    n_dir->inode = inode_num; //parent inode index
    n_dir->rec_len = EXT2_BLOCK_SIZE - 12;
    n_dir->name_len = 2;
    n_dir->file_type = EXT2_FT_DIR;//2
    strncpy(n_dir->name, "..", 2);

     fill_bitmap(free_block, sb, gd, 1);  //add 1 to proper postion of the block_bitmap
    //printf("test\n");

    //put back to parent dir block
     // Two situation, one is find enough space for new dir's rec_len in parent dir's block,
     //the other is not enough space, so new one more block for parent dir
     
     int enough = 0;
      //dir is the name for new dir
     int new_dir_len = align4(strlen(dir)) + 8;
     struct ext2_inode parent_inode = inode_table[inode_num - 1];
     inode_table[inode_num - 1].i_links_count += 1;
     //printf("parent_inode:%d\n", inode_num);
     int i;
     struct ext2_dir_entry_2* curr_entry;
     for(i = 0; i < 12 && parent_inode.i_block[i]; i++){
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
     //printf("i:%d\n",i);
     //printf("enough:%d\n",enough);

     //have enough space
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
        inode_table[inode_num - 1].i_size += EXT2_BLOCK_SIZE;
        inode_table[inode_num - 1].i_blocks += 2;
        inode_table[inode_num - 1].i_block[i+1] = new_free_block;




     }
     //common assgin

     curr_entry->inode = free_inode;
     curr_entry->name_len = strlen(dir);
     curr_entry->file_type = EXT2_FT_DIR; //2
     strncpy(curr_entry->name, dir, strlen(dir));

     
   return 0;

}

