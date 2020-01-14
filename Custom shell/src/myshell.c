#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

volatile int flag=0;

void parse(char *command, char **args);
void execute(char **args);
void execInput(char line[512]);
void trimTrailing(char *str);

//SIGALRM handler
void handle(int sig){ flag=0; }

int main(int argc, char **argv){

	char line[512];

	if(argc==1){
		//Interactive Mode
		printf("Inititalising Shell in Interactive Mode.\n");
		printf("Press 'quit' to exit\n");
			
		//print prompt
		do{ 
			printf("Custom_shell> ");
			if(fgets(line, sizeof(line), stdin)==NULL){
				perror("fgets");
				exit(1);
			}

			//remove "\n" character at the end of string
			line[strcspn(line, "\n")] = 0;
			execInput(line);
		//loop forever
		}while(1);
	}
	else if(argc==2){
		//Batch file mode
		FILE *fp;

		fp = fopen(argv[1] ,"r");
		if(fp == NULL){
			perror("fopen");
			exit(1);
		}
		printf("Inititalising Shell in Batch File Mode.\n");

		while(!feof(fp) && fgets(line, sizeof(line), fp)!=NULL){

			//skip if empty line
			if(strcmp(line,"\0") && strcmp(line, "\n") && strcmp(line, "\t") && strcmp(line, " ")){
				
				line[strcspn(line, "\r\n")] = 0;
				execInput(line);
			}
		}
		fclose(fp);
	}
	else{
		printf("ERROR:\nUsage: %s\nOr %s batchfile\n", argv[0], argv[0]);
	}

	return 0;
}

/*Remove trailing white space characters from string, if any
source http://codeforwin.org/2016/04/c-program-to-trim-trailing-white-space-characters-in-string.html */
void trimTrailing(char *command){
    
    int index, i;

    // Find last index of non-white space character
    i = 0;
    while(command[i] != '\0'){

        if(command[i] != ' '){
            index= i;
        }
        i++;
    }

    //Mark next character to last non-white space character as NULL
    command[index+1] = '\0';
}


/* execute--spawn a child process and execute the command.
from Lab Exercise 3 auth */
void execute(char **args){

	int pid, status;
	
	if(strcmp(args[0],"quit")==0){
		printf("Exiting...\n");
		//exit, no questions asked.
		exit(0);
	}

	//Get a child process.
	if ((pid = fork()) < 0) {
		perror("fork");
		exit(1);
	}
	
	//The child executes the command
	if (pid == 0) {

		execvp(*args, args);
		
		if(flag){
			//send signal to parent that command did not execute correctly
			kill(getppid(), SIGALRM);
		}
		exit(1);
	}

	//the parent waits
	else{
		while(wait(&status)!=pid);
	}
}


void parse(char *command, char **args){

	trimTrailing(command);

	while (*command != '\0') {

		while ((*command == ' ') || (*command == '\t') || (*command == '\n')){
			*command++ = '\0';
		}

		//Save the argument.
		*args++ = command;

		// Skip over the argument.
		while ((*command != '\0') && (*command != ' ') && (*command != '\t')){
			*command++;
		}
	}
	*args = NULL;
}


//This function takes a line from stdin or a file and executes it
void execInput(char line[512]){

	char *command;
	char *args[64];

	if(strstr(line, "&&")!=NULL){
		//for execution only if prev command was successful
		flag=1;
		command=strtok(line, "&&");
	}
	else{
		command=strtok(line, ";");
	}

	if(!flag){
		while(command != NULL){
			//execute sequentially, ignore return status of execvp
			parse(command, args);
        	execute(args);
			command=strtok(NULL, ";");
        }
	}
	else{
		//expect alarm in case of wrongful command input
		signal(SIGALRM, handle);
		//execute as long as previous command was executed sucessfully
		while(flag && command!=NULL){
			
			parse(command, args);
        	execute(args);
			command=strtok(NULL, "&&");
		}
	}
}