#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ext2.h"

int fd;
unsigned int* disk;
struct ext2_super_block *super_block;
struct ext2_group_desc *group_desc;
char* inode_bitmap;
char* block_bitmap;
struct ext2_inode* inode_table;
struct ext2_dir_entry_2*  block_table;

int next_path(char path[]);
int get_path_inode(char path[], int inode_num);
void print_inode();

#define READ_ONE_BLOCK(blocknum) (disk+ blocknum*EXT2_BLOCK_SIZE);
#define INODESIZE sizeof(struct ext2_inode);

