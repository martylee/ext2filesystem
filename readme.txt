Li Wan
g2marty
998801088

ext2_ls:
takes 2 parameters, the first is the name of an ext2 formatted virtual disk, the second parameter is the absolute path .the absolute path needs to start with "/", multiple "/" in path like "///home///User//" also works, otherwise, error message is printed.  "/"  for root directory.

ext2_rm:
takes 2 parameters, the first is the name of an ext2 formatted virtual disk, the second parameter is absolute path or get an error message.  The program used to track the path is same as ext2_ls. The program will check the path and ensure the target file exists in the path that is provided, or proper error message will be printed. It removes the file by adding its rec_len to previous rec_len, so the target file is skipped. Meanwhile, group descriptor, super_block and bitmap have also been modified according to the new structure. 

ext2_mkdir:
takes 2 parameters, the first is the name of an ext2 formatted virtual disk, the second parameter is absolute path or get an error message. To make a new dir,we need at least one new inode and one new dir, store the dir number in the new inode.i_block[0], then create suitable structure in the new block. When links the new inode to parent dir's block, previous rec_len is checked in order to have enough room to store the new dir's name. If not, find another empty block and links it to parent dir's inode.

ext2_cp:
takes 3 parameters, the first is the name of an ext2 formatted virtual disk, the second parameter is absolute path for the file on local machine, and the third is target path on image with the new file name. The key idea for cp is to read the local file by fopen, and calculate the target file size by fsize, then write the content of this file to free block by fread. The left part of this program is similar to ext2_mkdir(link the new node to its parent's dir block).

ext2_ln:
takes 3 parameters, the first is the name of an ext2 formatted virtual disk, the second parameter is the absolute path for the file in the image, and the third is the path in image which users want to link. The whole program can be considered as a easiler versiono of ext2_cp, since we only need to find the target file's inode and add it to the parent dir block in third parameter.

A3 DONE!







