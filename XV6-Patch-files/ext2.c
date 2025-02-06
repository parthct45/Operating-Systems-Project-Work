#define _LARGEFILE64_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
#include<linux/fs.h>
#include<sys/types.h>
#include<unistd.h>
#include<ext2fs/ext2_fs.h>
#include<ext2fs/ext2fs.h>
#include<stdint.h>
#include<math.h>
#include<string.h>

int *arr ;
int r = 0 ; 

struct ext2_inode getInode(int fd ,int inode_num , int inodes_per_group , int block_size , int inode_size){
	// Calculate the block group and the offset in table calculations
	struct ext2_group_desc bgdesc;
	struct ext2_inode inode ; 
	uint32_t bg_num ;
        uint32_t index ;

	
	
	if((inode_num %inodes_per_group)!=0){
		bg_num = inode_num / inodes_per_group ;
                index = (inode_num % inodes_per_group)-1 ;

        }
        else{
               	bg_num = (inode_num / inodes_per_group)-1 ;
                index = (inode_num-1) % inodes_per_group ;
        }
        off_t offset = 1024 + block_size + bg_num*sizeof(struct ext2_group_desc) ; 
	//printf("\n%ld\n" , offset) ; 
	lseek(fd , offset , SEEK_SET) ; 
	int temp = read(fd , &bgdesc ,sizeof(struct ext2_group_desc)) ;
	if(temp!=sizeof(struct ext2_group_desc)){
                printf("Error try reading again ") ;
		// Empty struct 
                return inode ;
        }
        int main_inode_block = bgdesc.bg_inode_table ;
	//printf("\n\n%d\n\n" , main_inode_block) ; 
        offset = main_inode_block *block_size + index*inode_size ;

        lseek64(fd , offset ,SEEK_SET) ;
        temp = read(fd , &inode , sizeof(struct ext2_inode))  ;

	//printf("\n%d\n" , inode.i_blocks) ; 
        if(temp!=sizeof(struct ext2_inode)){
                printf("Error try reading again");
		// Empty struct
                return inode ;
        }
	return inode ; 
}

int read_helper(int fd , int block_size , int block_number , int fin_level , int cur_level, int print) {
    uint32_t buf[200];
    lseek64(fd , block_number * block_size , SEEK_SET);
    read(fd , &buf , sizeof(buf));
    int count = 0 ;

    // Direct
    if (fin_level == cur_level) {
        for (int i = 0; i < 200 && buf[i] != 0; i++) {
            if(print == 1) 
		    printf("%d, ", buf[i]);
	    arr = (int *)realloc(arr, sizeof(int)*(r+1)) ; 
	    arr[r++] = buf[i] ;

        }
        return 0;
    }

    // Indirect
    for (int i = 0; i < 200 && buf[i] != 0; i++) {
        count++ ;
        if(print == 1)
		printf("(IND)%d, ",buf[i]);
        count+=read_helper(fd , block_size , buf[i], fin_level , cur_level + 1, print);
    }
    return count ;
}

void print_Inode(int fd, struct ext2_inode inode, int block_size, int print){
        if(print == 1){
        	printf("UID : %u \n" , inode.i_uid) ;
        	printf("GID : %u \n" , inode.i_gid) ;
        	printf("Mode : %u \n" , inode.i_mode);
        	printf("Size : %d \n" , inode.i_size) ;
        	printf("Generation : %u \n" , inode.i_generation);
//      	printf("Access time : %u \n" , inode.i_atime) ;
//      	printf("Inode Change time : %u \n" , inode.i_ctime);
//     	 	printf("Modification time : %u \n" , inode.i_mtime);
//      	printf("Deletion time : %u \n" , inode.i_dtime) ;
        	printf("Links count : %u \n", inode.i_links_count);
        	printf("Flags : %u \n", inode.i_flags);
        	printf("File ACL : %u \n" , inode.i_file_acl);
        	printf("Blockcount : %d \n", inode.i_blocks);
	}

        float fun = (float)inode.i_size ;
        int direct = (int)ceil(fun/ block_size) ;
        if (inode.i_blocks == 0){
                printf("No blocks alloted \n") ;
                return  ;
        }
        int i ;
        int indirect = 0 ;
        if(print == 1) printf("BLOCKS ->\n");
        for (i = 0; i < 12 && i<direct; i++) {
		if(print == 1)
                	printf("%d, ", inode.i_block[i]);
        }
        if (inode.i_block[12] != 0) {
                indirect++ ;
                if(print == 1) 
			printf("(IND)%d, ", inode.i_block[12]);
                indirect+=read_helper(fd, block_size, inode.i_block[12], 1, 1, print);
        }
        if (inode.i_block[13] != 0) {
                indirect++ ;
                if(print == 1) 
			printf("(IND)%d, ", inode.i_block[13]);
                indirect+=read_helper(fd , block_size , inode.i_block[13] , 2 , 1, print);
        }
        if (inode.i_block[14] != 0) {
                indirect++ ;
                if(print == 1) 
			printf("(IND)%d, ", inode.i_block[14]);
                indirect+=read_helper(fd , block_size , inode.i_block[14] , 3 , 1 , print);
        }
	if(print == 1){
        	printf("\nNumber of Direct blocks : %d | " , direct);
        	printf("Number of Indirect blocks : %d\n" , indirect);
        	printf("TOTAL(Data Blocks) : %d\n" , direct + indirect);
	}

	return ; 

}

void print_Data(int fd, struct ext2_inode inode, int block_size){
	float fun = (float)inode.i_size ;
        int direct = (int)ceil(fun/ block_size) ;
        if (inode.i_blocks == 0){
                printf("No blocks alloted \n") ;
                return  ;
        }

// Mode value for directory :16384
//Mode value for regular files :32768
	if (LINUX_S_ISREG(inode.i_mode)){
		uint8_t buffer[block_size];
        	ssize_t bytes_read;

		for(int i = 0; i <12 ; i++){
			lseek64(fd, inode.i_block[i]*block_size , SEEK_SET);
			bytes_read = read(fd, buffer, block_size);
			if (bytes_read == -1) {
        			perror("Read failed");
        			exit(errno);
			}
			if(bytes_read != block_size) buffer[bytes_read] = '\0';
			printf("%s" , buffer) ;
    		}
		for(int i = 0 ; i<r ;i++){
			lseek64(fd, arr[i]*block_size , SEEK_SET);
                        bytes_read = read(fd, buffer, block_size);
                        if (bytes_read == -1) {
                                perror("Read failed");
                                exit(errno);
                        }
			if(bytes_read != block_size) buffer[bytes_read] = '\0';
                        printf("%s" , buffer) ;

		}
	}

	// If directory file 
	else if(LINUX_S_ISDIR(inode.i_mode)){
		struct ext2_dir_entry con ; 
		for (int i = 0; i < 12 && i<direct; i++) {
                 	int block_num = inode.i_block[i] ;
                 	int main_off = block_num*block_size ;
                 	lseek(fd , main_off , SEEK_SET) ;
                 	off_t total_bytes_read = 0;
                 	while(total_bytes_read < block_size) {
                        	int temp = read(fd , &con , sizeof(struct ext2_dir_entry)) ;
                        	if(temp == -1){
                                	perror("Error ") ;
                                	exit(errno) ; 
                        	}
                        // End reached
                        	else if (temp == 0){
                                	break ;
                        	}
			        total_bytes_read += temp;
			        if(con.inode == 0) break ;	
                        	printf("Inode : %d\t ", con.inode) ;
                        	int qwerty = con.rec_len ;
                        	printf("Name : %s\t" , con.name) ;
				printf("\n");
            
                        	int offset = (-1*sizeof(struct ext2_dir_entry)) + con.rec_len ;
                        	lseek(fd , offset , SEEK_CUR) ;
                        }
                }
		for (int i = 0; i<r; i++){
			int block_num = arr[i] ;
                 	int main_off = block_num*block_size ;
                 	lseek(fd , main_off , SEEK_SET) ;
                 	off_t total_bytes_read = 0;
                 	while(total_bytes_read < block_size) {
                        	int temp = read(fd , &con , sizeof(struct ext2_dir_entry)) ;
                        	if(temp == -1){
                                	perror("Error ") ;
                                	exit(errno) ;
                        	}
                        // End reached
                       	 	else if (temp == 0){
                                	break ;
                        	}
                        	printf("Inode : %d\t ", con.inode) ;
                        	int qwerty = con.rec_len ;
                        	printf("Name : %s\t" , con.name) ;
                        	printf("\n");

                        	total_bytes_read += temp;

                        	int offset = (-1*sizeof(struct ext2_dir_entry)) + con.rec_len ;
                        	lseek(fd , offset , SEEK_CUR) ;
                        }

		}
	} 
	return ; 
}

int get_Inode_Num(int fd ,struct ext2_inode dir , char *path ,int block_size , int inodes_per_group , int inode_size){
	
	struct ext2_dir_entry con ; 
	char *name  ; 
	char *firstSlash = NULL ; 
	char *secondSlash = NULL ;
	int i ; 


	// Check the flags as well ; 
	// Process the path ; 
	firstSlash = strchr(path , '/') ;
        if(firstSlash!=NULL) {
                secondSlash = strchr(firstSlash + 1 , '/') ;
                if(secondSlash!= NULL) {
                        int length = secondSlash - (firstSlash + 1);
			name = malloc(length + 1); // Allocate memory for name
                        strncpy(name , firstSlash + 1 , length) ;
                        name[length] = '\0' ;
                }
                else{
                        name = strdup(path + 1) ;
                }
        }

	float fun = (float)dir.i_size ;
        int direct = (int)ceil(fun/ block_size) ;
        if (dir.i_blocks == 0){
                printf("No blocks alloted \n") ;
                return 0 ;
        }

	// Base case 
	if (secondSlash == NULL){
		for (i = 0; i < 12 && i<direct; i++) {
                 int block_num = dir.i_block[i] ;
                 int main_off = block_num*block_size ;
                 lseek(fd , main_off , SEEK_SET) ;
		 off_t total_bytes_read = 0;
                 while(total_bytes_read < block_size) {
                        int temp = read(fd , &con , sizeof(struct ext2_dir_entry)) ;
			if(temp == -1){
				perror("Error ") ; 
				return 0 ; 
			}
			// End reached
			else if (temp == 0){
				break ;
			}
                        //printf("Inode : %d ", con.inode ) ;
                        int qwerty = con.rec_len ;
                        //printf("Record length : %d " , con.rec_len) ;
                        //printf(" Name length : %d " , con.name_len)  ;
                        //printf("Name : %s " , con.name) ;
			
			total_bytes_read += temp;

                        if(strcmp(con.name , name) == 0)
                                return  con.inode ; 
			else{
				int offset = (-1*sizeof(struct ext2_dir_entry)) + con.rec_len ;
                                lseek(fd , offset , SEEK_CUR) ;
			    }
		 	}
		}
	}
	
	//Now read the directorys data blocks 
	for (i = 0; i < 12 && i<direct; i++) {
                 int block_num = dir.i_block[i] ;
		 int main_off = block_num*block_size ;
        	 lseek(fd , main_off , SEEK_SET) ;
		 off_t total_bytes_read = 0;
		
		 while(total_bytes_read < block_size) {
		 	int temp = read(fd , &con , sizeof(struct ext2_dir_entry)) ;
			if(temp == -1){
                                perror("Error ") ;
                                return 0 ;
                        }
                        // End reached
                        else if (temp == 0){
                                break ;
                        }

		 	//printf("Inode : %d ", con.inode ) ; 
		 	int qwerty = con.rec_len ;
			//printf("Record length : %d " , con.rec_len) ;
        		//printf(" Name length : %d " , con.name_len)  ;
        		//printf("Name : %s " , con.name) ;
			total_bytes_read += temp ; 

			if(strcmp(con.name , name) == 0){
				int inode_num = con.inode ; 
				dir = getInode(fd, inode_num , inodes_per_group , block_size , inode_size) ;
				// Truncate the path 
				return get_Inode_Num(fd , dir, secondSlash , block_size , inodes_per_group , inode_size) ; 
			}
			else{
				int offset = (-1*sizeof(struct ext2_dir_entry)) + con.rec_len ; 
				lseek(fd , offset , SEEK_CUR) ; 
			}
		 }
	}
	
	// O specifying invalid path ; 
	return 0 ; 

}

int main(int argc , char* argv[]){
        int fd ; 
	uint32_t file_inode_num;
        struct ext2_super_block sb ;
        struct ext2_group_desc bgd ;
	struct ext2_inode root_inode;  
        uint16_t inode_size ;
        int block_size ;
        uint32_t bg_num ;
        uint32_t index ;
	int inodes_per_group ; 


        if (argc !=4){
                printf(" format: ./inodenumber  device-file-name  path inode/data\n") ;
                return 0 ;
        }
	fd = open(argv[1] , O_RDONLY) ;
	char *path = argv[2] ; 
	if(fd == -1){
                perror("inodenumber:") ;
                exit(errno) ;
        }
	char *mode = argv[3] ; 
	// Skipping the boot block 
	lseek64(fd , 1024 , SEEK_CUR) ;
	
	// Reading the super block 
	int temp = read(fd , &sb , sizeof(struct ext2_super_block)) ;

	block_size = 1024 << sb.s_log_block_size ;
	inode_size = sb.s_inode_size ;
	inodes_per_group = sb.s_inodes_per_group ; 
	// I need to read block group 0 desc and inode 2 . 
	file_inode_num = 2 ;
	// But the index will be one 
	file_inode_num = 1 ; 
	lseek(fd , 1024 + block_size , SEEK_SET) ; 
	temp = read(fd , &bgd ,sizeof(struct ext2_group_desc)) ;
	if(temp!=sizeof(struct ext2_group_desc)){
                printf("Error try reading again ") ;
                return 0 ;
        }
	int inode_offset = bgd.bg_inode_table * block_size + file_inode_num*inode_size ;
	lseek(fd , inode_offset , SEEK_SET ) ; 
	temp = read(fd , &root_inode , sizeof(struct ext2_inode))  ;
        if(temp!=sizeof(struct ext2_inode)){
                printf("Error try reading again");
                return 0 ;
        }
	//printf("%d\n" , root_inode.i_blocks) ; 
	file_inode_num = get_Inode_Num(fd , root_inode, path , block_size , inodes_per_group , inode_size) ;
        if (file_inode_num == 0){
		printf("Entered path is not valid\n") ;
		return 1;
	}	
	struct ext2_inode inode = getInode(fd, file_inode_num, inodes_per_group, block_size, inode_size) ;
	//printf("................................................FINAL ...................................................................................") ; 
	//printf("%d" , file_inode_num) ;
	if(strcmp(mode, "inode") == 0){
		printf("INODE %d\n", file_inode_num) ;
		print_Inode(fd, inode, block_size, 1) ;
	}
	else{
		printf("Data :\n") ;
		print_Inode(fd, inode, block_size, 0) ;
		print_Data(fd, inode, block_size) ;
	}
	close(fd) ; 
	return 1 ;
}
