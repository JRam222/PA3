#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <sys/wait.h>
#include <fstream>
#include <fcntl.h>

using namespace std;
//
int builtIn(char **argv);
void parseSpace(char *line, char **argv);
void execute(char **argv);
int processString(char* line, char** argv, char** argvPiped, char** argvPiped2);
int parsePipe(char* line, char** strpiped);
int parseSemi(char* line, char** strsemi);
int parseRedir(char* line, char** strredir);
int parseApp(char* line, char** strapp);
void execArgsPiped(char** argv, char** argvPiped);
void execArgsPiped2(char** argv, char** argvPiped, char** argvPiped2);
void executeRedir(char **argv, char** argvPiped);
void executeApp(char** argv, char** argvPiped);


//used for executing append redirection
void executeApp(char** argv, char** argvPiped){
	int fd1, fd2;
	int dummy;
	if(fork()==0){

		fd2= open(argvPiped[0], O_RDWR | S_IRUSR | S_IWUSR | O_APPEND);
		if(fd2 < 0){
			perror("");
			exit(2);
		}

		if(dup2(fd2,1) != 1){
			perror("");
			exit(1);
		}close(fd2);

		execvp(argv[0],argv);
		perror("");
		exit(1);
	}else{
		wait(&dummy);
	}

}

//used for executing redirection
void executeRedir(char **argv, char** argvPiped){
	int fd1, fd2;
	int dummy;

	if(fork()==0){
	//	fd1=open("f1", O_RDONLY);
	//	if(fd1<0){
	//		perror("");
	//		exit(1);
	//	}

	//	if(dup2(fd1,0) != 0 ){
	//		perror("");
	//		exit(1);
	//	}close(fd1);

		fd2= open(argvPiped[0], O_WRONLY | O_TRUNC| O_CREAT,0644);
		if(fd2 < 0){
			perror("");
			exit(2);
		}

		if(dup2(fd2,1) != 1){
			perror("");
			exit(1);
		}close(fd2);

		execvp(argv[0],argv);
		perror("");
		exit(1);
	}else{
		wait(&dummy);
	}
}

//used for executing double piped commands
void execArgsPiped2(char** argv, char** argvPiped, char** argvPiped2){
	int status;
	int i;

	int pipes[4];
	pipe(pipes);
	pipe(pipes + 2);

	if(fork()==0){
		dup2(pipes[1],1);
		close(pipes[0]);
		close(pipes[1]);
		close(pipes[2]);
		close(pipes[3]);
		
		execvp(argv[0],argv);
	}else{
		if(fork() == 0){
			dup2(pipes[0],0);

			dup2(pipes[3],1);

			close(pipes[0]);
			close(pipes[1]);
			close(pipes[2]);
			close(pipes[3]);

			execvp(argvPiped[0],argvPiped);
		}else{
			if(fork() ==0){
				dup2(pipes[2],0);

				close(pipes[0]);
				close(pipes[1]);
				close(pipes[2]);
				close(pipes[3]);
				
				execvp(argvPiped2[0],argvPiped2);
			}
		}
	}
	
	close(pipes[0]);
	close(pipes[1]);
	close(pipes[2]);
	close(pipes[3]);
	
	for(int k=0; k<3;k++){
		wait(NULL);
	}
}

//used for executing single piped commands
void execArgsPiped(char** argv, char** argvPiped){
	int pipefd[2];
	pid_t p1,p2;
	pipe(pipefd);

	p1=fork();
	if(p1<0){
		printf("\nCould not fork");
		return;
	}

	if(p1==0){
		close(pipefd[0]);
		dup2(pipefd[1],STDOUT_FILENO);
		close(pipefd[1]);
		if(execvp(argv[0],argv)<0){
			printf("\nCould not execute command 1");
			exit(0);
		}
	}else{
		close(pipefd[1]);
		p2 =fork();

		if(p2<0){
			printf("\nCould not fork");
			return;
		}

		if(p2==0){
			dup2(pipefd[0],STDIN_FILENO);
			close(pipefd[0]);
			if(execvp(argvPiped[0],argvPiped) < 0){
				printf("\nCould not execute command 2");	
				exit(0);
			}
		}else{
			wait(NULL);
			wait(NULL);
		}
	}
}

//parses append sign
int parseApp(char* line, char** strapp){
	for(int k=0; k<2;k++){
		strapp[k] = strsep(&line, ">>");
		if(strapp[k] == NULL)
			break;
	}

	if(strapp[1] == NULL)
		return 0; // then no append
	else
		return 1;
}

//parses semi colon
int parseSemi(char* line, char** strsemi){
	for(int k=0; k<2;k++){
		strsemi[k] = strsep(&line, ";");
		if(strsemi[k] == NULL)
			break;
	}

	if(strsemi[1] == NULL)
		return 0; // then no semicolon
	else
		return 1;
}

//parses pipes
int parsePipe(char* line, char** strpiped){
	for(int k=0; k<3;k++){
		strpiped[k] = strsep(&line, "|");
		if(strpiped[k] == NULL)
			break;
	}

	if(strpiped[1] == NULL){
		return 0; // then no pipe
	}
	else if(strpiped[2] != NULL){
		return 2;
	}else{ 
		return 1;
	}
}

//parses redirect symbols
int parseRedir(char* line, char** strredir){
	for(int k=0; k<3;k++){
		strredir[k] = strsep(&line, ">");
		if(strredir[k] == NULL)
			break;
	}

	if(strredir[1] == NULL){
		return 0; // then no redir
	}else if (strredir[2] != NULL){
		return 2;
	}else{ 
		return 1;
	}
	
}

//parses spaces 
void parseSpace(char *line, char **argv){
	for(int i =0; i< 7;i++){
		argv[i] = strsep(&line, " ");

		if(argv[i] == NULL)
			break;
		if(strlen(argv[i]) == 0 || argv[i] == "\n"){
			argv[i]=NULL;
			i--;
		}
	}
}

//executes command
void execute(char **argv){
	pid_t pid= fork();
	
	if(pid < 0){
		printf("forking failed\n");
		return;
	}else if(pid ==0){
		if(execvp(argv[0],argv) < 0){
			printf("exec failed \n");			
		}
		exit(0);
	}else{
		wait(NULL);
		return;
	}
	
}

//execute built in commands
int builtIn(char** argv){
	int trigger =0;
	char* builtIn[4];
	string exit = "exit";
	string cd = "cd";
	string help = "help";
	builtIn[0] = (char*)"exit";
	builtIn[1] = (char*)"cd";	
	builtIn[2] = (char*)"help";

	for(int k=0; k<3;k++){
		if(strcmp(argv[0],builtIn[k]) == 0 ){
			trigger = k+1;
			break;	
		}
	}

	switch(trigger){
	case 1:
		std::exit(0);
	case 2:
		chdir(argv[1]);
		return 1;
	case 3:
		break; // open help method
		return 1;
	default:
		break;
	}
	return 0;
}

//processes input line and directs the flow of program
int processString(char* line, char** argv, char** argvPiped, char** argvPiped2){
	char* strpiped[3];
	char* strsemi[2];
	char* strredir[3];
	int piped =0;
	int semi =0;
	int redir =0;
	semi = parseSemi(line,strsemi);
	piped = parsePipe(line,strpiped);
	redir = parseRedir(line,strredir);
	if(piped==1){
		parseSpace(strpiped[0], argv);
		parseSpace(strpiped[1], argvPiped);
	}else if (piped ==2){
		parseSpace(strpiped[0], argv);
		parseSpace(strpiped[1], argvPiped);
		parseSpace(strpiped[2], argvPiped2); 
		return 3;
	}else if(semi ==1){
		parseSpace(strsemi[0],argv);
		execute(argv);
		parseSpace(strsemi[1],argv);
		execute(argv);
		return 0;
	}else if (redir ==1){
		parseSpace(strredir[0],argv);
		parseSpace(strredir[1],argvPiped);
		executeRedir(argv,argvPiped);
		return 0;
	}else if(redir ==2){
		parseSpace(strredir[0],argv);
		parseSpace(strredir[2],argvPiped);
		executeApp(argv,argvPiped);
		return 0;
	}else{
		parseSpace(line,argv);
	}

	if(builtIn(argv))
		return 0;
	else
		return 1 + piped;
}

int main(){
	char line[1024];	//input line
	char *argv[10];
	char* argvPiped[10];
	char* argvPiped2[10];
	int pipe =0;	
	
	while(1){
		printf("mysh >> ");
		fgets(line,sizeof(line),stdin);
		char *pos;
		if((pos=strchr(line, '\n')) != NULL) // remove new line of input
			*pos = '\0';
		pipe = processString(line, argv, argvPiped,argvPiped2);
		if(pipe ==1)
			execute(argv);

		if(pipe ==2){
			execArgsPiped(argv,argvPiped);		
		}

		if(pipe ==3){
			execArgsPiped2(argv, argvPiped, argvPiped2);	
		}
			
	}
	return 0;
}


