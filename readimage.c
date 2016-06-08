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

#define BASE_OFFSET 1024  /* location of the super-block in the first group */

#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*1024)

void printBits(unsigned char block){
    
    for (int i = 0; i < 8; i++){
        printf("%d",(block >> i)& 1);
    }
    printf(" ");
}



int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024*2);
    char* blockbitmap = (char*)(disk + (gd->bg_block_bitmap)* EXT2_BLOCK_SIZE);
    char* inodebitmap = (char*)(disk + (gd->bg_inode_bitmap)* EXT2_BLOCK_SIZE);

    

    
    
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    printf("Block group:\n");
    printf("    block bitmap: %d\n",gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n",gd->bg_inode_bitmap);
    printf("    inode table: %d\n",gd->bg_inode_table);
    printf("    free blocks: %d\n",sb->s_free_blocks_count);
    printf("    free inodes: %d\n",sb->s_free_inodes_count);
    printf("    used_dirs: %d\n",gd->bg_used_dirs_count);
    
    //TE8
    printf("Block bitmap: ");
    for (int i = 0; i < sb->s_blocks_count/8; ++i)
    {
        printBits(blockbitmap[i]);
    }
    
    printf("\nInode bitmap: ");
    for (int i = 0; i < sb->s_inodes_count/8; ++i)
    {
        printBits(inodebitmap[i]);
    }
    printf("\n");
    printf("\nInodes:\n");

    struct ext2_inode* inode_table = (struct ext2_inode*)(disk + (gd->bg_inode_table * EXT2_BLOCK_SIZE));
    for (int i = 0; i < sb->s_inodes_count; ++i)
    {   
        if (inode_table[i].i_size <= EXT2_BLOCK_SIZE && inode_table[i].i_size){ // not empty
            unsigned short mode = inode_table[i].i_mode;
            char *type = malloc(sizeof(char));
            if (mode & EXT2_S_IFREG){
                strcat(type,"f"); 
            }if (mode & EXT2_S_IFDIR){
                strcat(type,"d");
            }
            printf("[%d] type: %s size: %d links: %d blocks: %d\n",i+1,type,inode_table[i].i_size,inode_table[i].i_links_count,
            inode_table[i].i_blocks);
            printf("[%d] Blocks:  ",i+1);
            for (int idx = 0; idx < 16; ++idx)
            {
                int num = inode_table[i].i_block[idx];
                if (num == 0){
                    break;
                }
                //debug
                //printf("%d",idx);
                printf("%d",num);
                
            printf("\n");
            }


            //struct ext2_dir_entry_2* entry = (struct ext2_dir_entry_2*) inode_table[i].i_block;
            





        }
    }

    //TE9
    printf("\nDirectory Blocks:\n");
    for (int i = 0; i < sb->s_inodes_count; ++i)
    {   
        if (inode_table[i].i_size <= EXT2_BLOCK_SIZE && inode_table[i].i_size){ // not empty
            unsigned short mode = inode_table[i].i_mode;
            char *type = malloc(sizeof(char));
            if (mode & EXT2_S_IFREG){
                strcat(type,"f"); 
            }if (mode & EXT2_S_IFDIR){
                strcat(type,"d");
            }
            
            if (strcmp(type,"d") == 0){
               
                for (int idx = 0; idx < 16; ++idx)
                {
                    int num = inode_table[i].i_block[idx];
                    if (num == 0){
                        break;
                    }
                    printf("   DIR BLOCK NUM: %d (for inode %d)\n",num,i+1);
                
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
                    
                        char file_name[255+1];
                        memcpy(file_name, entry->name, entry->name_len);
                        file_name[entry->name_len] = 0;
                        char* ftype = malloc(sizeof(char));;
                        if (entry->file_type == 1){
                            strcat(ftype,"f");
                        }
                        if(entry->file_type == 2){
                            strcat(ftype,"d");
                        }

                        
                        printf("Inode: %d rec_len: %u name_len: %d type= %s name=%s\n",
                            entry->inode,entry->rec_len,entry->name_len,
                            ftype,file_name);
                        //printf("\n");
                        //printf("%d\n",entry->inode);
                        //printf("%s\n",file_name);
                        //printf("%u\n",entry->rec_len);
                        size += entry->rec_len;
                        entry = (void *)entry + entry->rec_len;
               
                    }
            
                }
            }



            //struct ext2_dir_entry_2* entry = (struct ext2_dir_entry_2*) inode_table[i].i_block;
            





        }
    }
    
    
    
    
    
    //free(type);
    return 0;

}
