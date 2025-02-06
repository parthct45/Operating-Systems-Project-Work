#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<limits.h>
#include<stdlib.h>
#include<fcntl.h> 
#include<sys/wait.h> 

char PS1[234] ;
char PATH[] = "/home/parth/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin" ; 


int mode = 0 ; // mode for PS1 

char * error ; 

void setPath(char ans[]){
        // Spliting at =
        char * help = strtok(ans , "=") ;
        help = strtok(NULL , "=") ;
        int i = 0 ;
        for(i ; i<strlen(help) ; i++){
                PATH[i]=help[i] ;
        }
        PATH[i] = '\0' ;

}

void setPrompt(char ans[]){
        // Spliting at =
        char * help = strtok(ans , "=") ;
        help = strtok(NULL , "=") ;
        if(strcmp(help , "\"\\w$\"") == 0){
               mode =0 ;
               return ;
        }
        help = help +1 ;
        int i = 0 ;
        for(i ; i<strlen(help)-1 ; i++){
                PS1[i]=help[i] ;
        }

        // Error handling over here

        if(help[strlen(help)-1] != '\"'){
                printf("Error : Usage > PS1=\"xyz\"\n") ;
                return ;
        }

        PS1[i] = '\0' ;

        mode = 1 ;
}


char* executablePath(char *command) {
    if(strchr(command , '.')!=NULL  && strchr(command ,'/')!=NULL){
	    char * temp = command + 2  ; 
	    if(access(temp , X_OK) == 0){
		    return strdup(command) ; 
	    }
	    else{
		    printf("Error : No such file or directory\n") ; 
		    return error ; 
	    }
    }
	
    // if already executable path is given 
    if(strchr(command , '/')!=NULL){
	    if(access(command , X_OK) == 0){
		    return strdup(command) ;
	    }
	    else{
		    printf("Error : No such file or directory\n" ) ;
		    return error ; 
	    }
    }



    char *path = strdup(PATH); 
    // Spliting at : 
    char *portion= strtok(path, ":"); 
    while (portion != NULL) {
        char *executable_path ; 
	asprintf(&executable_path , "%s/%s" , portion , command) ; 


	// Check if the path specified(file) has the executable permissions 

        if (access(executable_path, X_OK) == 0) {
            free(path);
            return strdup(executable_path);
        }
        portion = strtok(NULL, ":");
    }

    free(path);
    return NULL;
}

void handleRedirection(char* filename , char symbol[] ){ 
								
	if(symbol[0] == '>' && symbol[1] == '>'){
		int fd = open(filename , O_WRONLY | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR) ; 
		if(fd == -1){
			printf("Sorry error occured\n") ; 
			return ; 
		}
		dup2(fd, 1) ;
		close(fd) ; 
	}
	else if(symbol[0] == '>'){
		int fd = open(filename , O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR) ; 
		if(fd == -1){
			printf("Sorry error occured\n") ; 
			return ; 
		}
		dup2(fd, 1);
		close(fd) ; 
	}

	else if(symbol[0] == '<'){
		int fd = open(filename , O_RDONLY) ; 
		if(fd == -1){
			printf("%s not present\n" , filename) ; 
			return ;
		}	
		dup2(fd, 0) ;
	        close(fd) ;
	}	




}


int main(){
	int pid = INT_MAX ; 
	char program[128] ;

	char off[] = "exit" ; 
	
	char buf[1024] ;

	char *args[25] ;
	// Maximum 25 arguments 	
	
	char symbol[2] ; 
	char *rFile ; 


	// Useful for restoring the original file descriptors 
	int original_stdout ; 
	int original_stdin ; 
	
	error = (char*)malloc(sizeof(char)) ; 

	while(1){

		// default mode
		if(mode == 0){
			getcwd(PS1,sizeof(PS1)) ; 
			mode = 0 ;
		}
		
		printf("%s $ " , PS1) ; 
		for(int i = 0 ; i<25 ; i++){
                	args[i] = (char*)malloc(sizeof(char)*100) ;
                	args[i] = NULL ;
        	}

		for(int i =0 ; i<2 ; i++){
			symbol[i] = '0' ; 
		}

		char * result = fgets(buf , sizeof(buf) , stdin) ;

		// Ctrl + D 	
		 if (result == NULL) {
			printf("\n") ; 
			break ; 
    		}
		

		size_t len =strlen(buf) ; 

		
		// Handled enter pressed case 
		if(strlen(buf) == 1) continue ; 
		
		
		if(len >0 && buf[len-1] == '\n'){
			buf[len-1] = '\0' ; 
		}


                if(strcmp(buf, off)==0){
                        break ;
		} 

		char * portion ; 	
		portion = strtok(buf , " ") ; // At space split 

		int counter =0 ;
	        int point = 0 ; 	
		while(portion!=NULL){
			if(point == 1){
                                // Handle redirection
                                rFile = portion ;
                                original_stdout = dup(1);
				original_stdin = dup(0) ; 
                                handleRedirection(rFile , symbol);
                                break ;
                        }


			if(strcmp(portion , ">") == 0 || strcmp(portion , ">>") == 0 || strcmp(portion , "<") == 0){
				point = 1 ;
				symbol[0] = portion[0] ; 
				if(strlen(portion) == 2){
					symbol[1] = portion[1] ;
				}
			}
			
			else{
				args[counter] = strdup(portion) ; 
				counter++ ;
			}
			portion = strtok(NULL , " ") ;
		}

                if(args[0][0] == 'P' && args[0][1] == 'S' && args[0][2] == '1' && args[0][3] == '=' && args[0][4] == '\"'){
                        setPrompt(buf) ;
                        continue ;
                }

                if(args[0][0] == 'P' && args[0][1] == 'A' && args[0][2] == 'T' && args[0][3] == 'H'&& args[0][4] == '='){
                        setPath(buf) ;
                        continue ;
                }
 
                if(strcmp(args[0] , "cd") == 0){
			if(args[1] == NULL){
				args[1] = strdup("/home/parth") ; 
			}
                        int fun = chdir(args[1]) ;
                        if(fun!=0){
                               perror("Error") ;
                        }
                        continue ;
                }

		char * path = executablePath(args[0]) ; 

		if(path == NULL){

			// Error handling done over here 
			printf("Error : %s command not found \n", args[0])  ;
		       	continue ; 	
		}	

		if(path == error){
			continue ; 
		}

		if(strcmp(args[0] , "echo") == 0 && (strcmp(args[1] , "$PATH") == 0 || strcmp(args[1], "\"$PATH\"")==0)){
			printf("%s\n",PATH) ; 
			continue ; 
		}

		// Child processes inherit the file descriptors as they are 
	       	pid = fork(); 
                if(pid == 0) {
		    //If commmand exec fails then  print the error 
                    execv(path, args);
		    perror("Error") ;  
		    return 0 ; 
                } 
		else {
                     wait(0);
                } 


		// Restoring to defaults 
		dup2(original_stdout , 1) ; 
		dup2(original_stdin , 0) ; 

		
        }
        printf("Exiting the shell\n");
	printf("Thank you !\n") ;  
	return 0 ; 
}

