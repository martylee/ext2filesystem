#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "ext2_lib.h"

void init(char* dir){
    fd = open(dir, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
	    exit(1);
    }

    struct ext2_super_block *super_block = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *group_desc = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE*2);
    char* block_bitmap = (char*)(disk + (group_desc->bg_block_bitmap)* EXT2_BLOCK_SIZE);
    char* inodebitmap = (char*)(disk + (group_desc->bg_inode_bitmap)* EXT2_BLOCK_SIZE);
    struct ext2_inode* inode_table = (struct ext2_inode*)(disk + (group_desc->bg_inode_table * EXT2_BLOCK_SIZE));
    
    //getbitmap();
    //read_inode_table_n_blocks();
}


int bitmapToBlock(char* bitmap, unsigned int index){
    unsigned int arrayPosition = index / 8 ;
    unsigned int shiftPosition = index % 8;
    return (bitmap[arrayPosition] >> shiftPosition) & 1;
}

void read_inode_table_n_blocks(){
   int i;
   inode_table = (struct ext2_inode*)READ_ONE_BLOCK(group_desc->bg_inode_table);
   int count = super_block ->s_inodes_count;
   for (i = 0; i < count; i++){   
        if (bitmapToBlock(inode_bitmap,i )){ // not empty
            unsigned short mode = inode_table[i].i_mode;
            char *type = malloc(sizeof(char));
            if (mode & EXT2_S_IFREG){
                strcat(type,"f"); 
            }if (mode & EXT2_S_IFDIR){
                strcat(type,"d");
            }
            printf("[%d] type: %s size: %d links: %d blocks: %d \n",i+1,type,inode_table[i].i_size,inode_table[i].i_links_count,
            inode_table[i].i_blocks);
            printf("[%d] Blocks:  ",i+1);
            int idx;
            for(idx = 0; idx < inode_table[i].i_blocks+1; idx++)
            {
                int num = inode_table[i].i_block[idx];
                if (num == 0){
                    break;
                }else{
                    
                    block_table[idx+i] = *(struct ext2_dir_entry_2*)(inode_table[i].i_block+idx);
                }
                printf("%d",num);
            }
            printf("\n");
        }
    }
}

/*for both inode and block bitmaps*/
//void getbitmap(char* data){
   // inode_bitmap = (char*)READ_ONE_BLOCK(group_desc->bg_inode_bitmap);
   // block_bitmap = (char*)READ_ONE_BLOCK(group_desc->bg_block_bitmap);
//}



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

int get_path_inode(char path[], int inode_num){
	 char new_path[strlen(path)+1];
	 memcpy(new_path, path, strlen(path));
     new_path[ strlen(path)] = 0;
     int pos;
	 
	 while ((pos = next_path(new_path) ) > 0){
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
		printf("%s\n",new_path);
		printf("%s\n",dir);
	}
	return 1;
}


