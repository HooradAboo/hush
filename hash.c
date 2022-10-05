#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUFF_SIZE 512
#define TOK_SIZE 128
#define TOK_DELIM " \t\r\n\a"

#define WHITE   "\x1B[37m"
#define GREEN   "\x1b[32m"
#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"


char* username () {
    return getenv("USER");
}


void remove_space (char* buff) {
    // Remove white space
    if(buff[0] == ' ' || buff[0] == '\n') {
        memmove(buff, buff+1, strlen(buff));
    }
    if(buff[strlen(buff)-1] == ' ' || buff[strlen(buff)-1] == '\n') {
        buff[strlen(buff)-1] = '\0';
    }
}


void parse (char *line, char** args, int *argc, const char* delim) {
	char *arg;
	arg = strtok(line, delim);
	int pc = -1;

	while(arg) {
		args[++pc] = malloc(sizeof(arg)+1);
		strcpy(args[pc], arg);
        remove_space(args[pc]);
		arg = strtok(NULL,delim);
	}
	args[++pc] = NULL;
	*argc = pc;
}


void execute (char** args) {
    pid_t pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror(RED "Invalid input");
        } 
    } else if (pid == -1) {
        perror(RED "Invalid input");
        return;

    } else {
        wait(NULL);
    }
} 


void execute_pipe (char* line) {
    char* pipes[BUFF_SIZE];
    int pipec;
    parse (line, pipes, &pipec, "|");

    char* args[BUFF_SIZE];
    int argc;
    int i;

    pid_t pid;

    // fd[0] will be the fd(file descriptor) for the read end of pipe 
    // and fd[1] will be the fd for the write end of pipe.
    int fd[pipec][2];     

    for (i = 0; i < pipec; i++) {
        parse(pipes[i], args, &argc, " \n");

        // if(strchr)

        if(i != pipec - 1) {          // Not last pipe
			if(pipe(fd[i]) < 0) {
				perror(RED "Pipe creating was not successfull\n");
				return;
			}
		}
        
        pid = fork();

        if (pid == 0) { // Chile process
            if (i == 0) {                       // Firs pipe - no input
                dup2(fd[i][1], 1);
				close(fd[i][0]);
				close(fd[i][1]);

            } else if (i == pipec - 1) {     // Last pipe - no output
                dup2(fd[i-1][0], 0);
				close(fd[i-1][1]);
				close(fd[i-1][0]);

            } else {                            // Middle pipes
                dup2(fd[i-1][0], 0);
				close(fd[i-1][1]);
				close(fd[i-1][0]);
                dup2(fd[i][1], 1);
				close(fd[i][0]);
				close(fd[i][1]);
            }
            
            if (execvp(args[0], args) == -1) {
                perror(RED "Invalid input"); 
                return;
            } 
                      
            
        } else if (pid == -1) {
            perror(RED "Invalid input");
            return;
        }
    }
    close(fd[i-1][0]);
    close(fd[i-1][1]);
    wait(NULL);
} 

// void execute_pipe(char*line){//can support up to 10 piped commands
//     char* buf[BUFF_SIZE];
//     int nr;
//     parse (line, buf, &nr, "|");

// 	if(nr>10) return;
	
// 	int fd[10][2],i,pc;
// 	char *argv[100];

// 	for(i=0;i<nr;i++){
// 		parse(buf[i], argv, &pc, " \n");
// 		if(i!=nr-1){
// 			if(pipe(fd[i])<0){
// 				perror("pipe creating was not successfull\n");
// 				return;
// 			}
// 		}
        
// 		if(fork()==0){//child1
// 			if(i!=nr-1){
// 				dup2(fd[i][1],1);
// 				close(fd[i][0]);
// 				close(fd[i][1]);
// 			}

// 			if(i!=0){
// 				dup2(fd[i-1][0],0);
// 				close(fd[i-1][1]);
// 				close(fd[i-1][0]);
// 			}
// 			execvp(argv[0],argv);
// 			perror("invalid input ");
// 			exit(1);//in case exec is not successfull, exit
// 		}
// 		//parent
// 		if(i!=0){//second process
// 			close(fd[i-1][0]);
// 			close(fd[i-1][1]);
// 		}
// 		wait(NULL);
// 	}
// }


void execute_redirect (char** cmds, int pipe_num) {
    char* args[BUFF_SIZE];
    int argc;
    int i;
    int fd;    

    parse(cmds[i], args, &argc, " \n");

    if (fork() == 0) { // Chile process
        fd = open(cmds[1], O_WRONLY | O_CREAT, 777);

        if (fd < 0) {
            perror (RED "Cannot open file");
            return;
        }

        dup2(fd,1);
        
        execvp(args[0], args);
        perror(RED "Invalid input");
        exit(1);
    } 

    wait(NULL);
}


int main() {
    char dir[BUFF_SIZE];
    char host[BUFF_SIZE];
    char line[500];
    char* args[BUFF_SIZE];
    int argc = 0;
    int i;

    while(1) {
        gethostname(host, sizeof(host));
        getcwd(dir, BUFF_SIZE);
        printf(GREEN "%s@%s" WHITE ":" BLUE "%s$ " WHITE, username(), host, dir);

        // read user input
        fgets(line, 500, stdin);

        if (strchr(line, '|')) {                 // pipe commands
            // printf("pipe %s\n", line);
            // parse(line, args, &argc, "|");
            execute_pipe(line);
            // execute_pipe(args, argc);

        } else if (strchr(line, '>')) {                 // redirect output to file
            // printf("rediret %s\n", line);
            parse(line, args, &argc, ">");
            execute_redirect(args, argc);

        } else {                            // single command
            // printf("single command %s\n", line);
            parse(line, args, &argc, " \n");
            execute(args);  
        }
        sleep(1);
        // for (i = 0; i < argc; i++) {
        //         printf("%s\n", args[i]);
        // }
    }
}