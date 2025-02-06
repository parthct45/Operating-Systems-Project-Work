#define _LARGEFILE64_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
#include<linux/fs.h>
#include<sys/types.h>
#include<unistd.h>
#include<ext2fs/ext2_fs.h>
#include<stdint.h>
#include<math.h>
 

int read_helper(int fd , int block_size , int block_number , int fin_level , int cur_level) {
    uint32_t buf[200];
    lseek64(fd , block_number * block_size , SEEK_SET);
    read(fd , &buf , sizeof(buf));
    int count = 0 ; 
    
    // Direct 
    if (fin_level == cur_level) {
        for (int i = 0; i < 200 && buf[i] != 0; i++) {
            printf("%d, ", buf[i]);
        }
        return 0;
    }
    
    // Indirect
    for (int i = 0; i < 200 && buf[i] != 0; i++) {
	count++ ; 
	printf("(IND)%d, ",buf[i]);
        count+=read_helper(fd , block_size , buf[i], fin_level , cur_level + 1);
    }
    return count ; 
}

int main(int argc , char* argv[]){
        int fd , inode_num;
        struct ext2_super_block sb ;
	struct ext2_group_desc bgd ;
        struct ext2_inode inode ;
        uint16_t inode_size ;
        int block_size ;
        uint32_t bg_num ;
        uint32_t index ; 


	if (argc !=3){
		printf(" format: ./inodenumber  device-file-name  inode-number\n") ;
		return 0 ; 
	}


        inode_num = atoi(argv[2]) ;
        fd = open(argv[1] , O_RDONLY) ;
        if(fd == -1){
                perror("inodenumber:") ;
                exit(errno) ;
        }
	if(inode_num == 0) return 0 ;
       		
        // Skipping the boot block 
        lseek64(fd , 1024 , SEEK_CUR) ;

        int temp = read(fd , &sb , sizeof(struct ext2_super_block)) ;
	block_size = 1024 << sb.s_log_block_size ;
	inode_size = sb.s_inode_size ;
        if((inode_num %sb.s_inodes_per_group)!=0){
                bg_num = inode_num / sb.s_inodes_per_group ;
		index = (inode_num % sb.s_inodes_per_group)-1 ;

        }
        else{
                bg_num = (inode_num / sb.s_inodes_per_group)-1 ;
		index = (inode_num-1) % sb.s_inodes_per_group ; 
        }
        lseek64(fd , 1024 + block_size + bg_num*sizeof(struct ext2_group_desc) , SEEK_SET) ;

        // Read the group descriptor of the intended block group number         
        temp = read(fd , &bgd ,sizeof(struct ext2_group_desc)) ;
        if(temp!=sizeof(struct ext2_group_desc)){
                printf("Error try reading again ") ;
		return 0 ; 
	}
        int inode_offset = bgd.bg_inode_table*block_size + index*inode_size ;

        lseek64(fd , inode_offset ,SEEK_SET) ;
        temp = read(fd , &inode , sizeof(struct ext2_inode))  ;
	if(temp!=sizeof(struct ext2_inode)){
		printf("Error try reading again");
		return 0 ; 
	}
	printf("Inode %d: \n" , inode_num) ;
	printf("UID : %u \n" , inode.i_uid) ;
        printf("GID : %u \n" , inode.i_gid) ; 	
	printf("Mode : %u \n" , inode.i_mode);
	printf("Size : %d \n" , inode.i_size) ;
	printf("Generation : %u \n" , inode.i_generation);	
//	printf("Access time : %u \n" , inode.i_atime) ;
//	printf("Inode Change time : %u \n" , inode.i_ctime);
//	printf("Modification time : %u \n" , inode.i_mtime);
//	printf("Deletion time : %u \n" , inode.i_dtime) ; 
	printf("Links count : %u \n", inode.i_links_count);
	printf("Flags : %u \n", inode.i_flags);
	printf("File ACL : %u \n" , inode.i_file_acl);
	printf("Blockcount : %d \n", inode.i_blocks);

	float fun = (float)inode.i_size ; 
	int direct = (int)ceil(fun/ block_size) ; 
	if (inode.i_blocks == 0){
		printf("No blocks alloted \n") ;
		return 0 ; 
	}
        int i ;	
	int indirect = 0 ; 
	printf("BLOCKS ->\n");  
	for (i = 0; i < 12 && i<direct; i++) {
        	printf("%d, ", inode.i_block[i]);
    	}
	if (inode.i_block[12] != 0) {
		indirect++ ; 
        	printf("(IND)%d, ", inode.i_block[12]);
        	indirect+=read_helper(fd, block_size, inode.i_block[12], 1, 1);
    	}
	if (inode.i_block[13] != 0) {
		indirect++ ; 
        	printf("(IND)%d, ", inode.i_block[13]);
        	indirect+=read_helper(fd , block_size , inode.i_block[13] , 2 , 1);
    	}
	if (inode.i_block[14] != 0) {
		indirect++ ; 
        	printf("(IND)%d, ", inode.i_block[14]);
        	indirect+=read_helper(fd , block_size , inode.i_block[14] , 3 , 1);
    	}
	printf("\nNumber of Direct blocks : %d | " , direct);
	printf("Number of Indirect blocks : %d\n" , indirect);
	printf("TOTAL(Data Blocks) : %d\n" , direct + indirect); 
	close(fd) ;

}

