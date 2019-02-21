/*
 * Craig Mathieson
 * January 25, 2019
 */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>

//Prototypes
void initialise();
void queryUser();
void get_args(char *input);
void run_cmd();
void see_directory(char *current_dir);
void go_home();
void go_to_dir(char *current_dir, char *dir);
void delete_var(char *var);
void purgeArgs();
void make_var(char *var, char *value);
void execute();
void INTHandler(int signal);

//Global Variables
char *argv[512];
int argc = 0;
pid_t pid = 1;

int main()
{
    int run = 1;

    signal(SIGINT, INTHandler);

    initialise();

    while(run){

        queryUser();
        //after processing set argc back to 0
        purgeArgs();
        argc = 0;

    }

    return 0;
}

void INTHandler(int signal){
    if(pid <= 0) {
        printf("Ctrl-C pressed: exiting child process\n");
        exit(0);
    }
}

void purgeArgs(){
    int i = 0;

    while(i <= 511) {
        argv[i] = NULL;
        i++;
    }
}

void initialise()
{

    char *home = getenv("HOME");
    char full_path[260];
    strcpy(full_path, home);
    strcat(full_path, "/.kapishrc");

    FILE *initFile = fopen(full_path, "r");

    if(initFile == NULL) {
        fprintf(stderr, "ERROR: no .kapishrc file");
        exit(EXIT_FAILURE);
    }

    for(;;){
        char input[514];
        purgeArgs();
        argc = 0;

        //if EOF reached; return
        if(fgets(input,514,initFile) == NULL){
            fclose(initFile);
            return;
        }

        printf("? %s", input);

        //check if over 512
        if (strchr(input, '\n') == NULL) {
            fprintf(stderr, "ERROR: Input too long!\n");
            //get rid of extra input
            int c;
            while((c = getc(stdin)) != '\n' && c != EOF);
            continue;
        }

        //tokenize into arguments
        get_args(input);
    }
}

void execute(){

    int status;

    int count = 0;

    while(argv[count] != NULL) {
        strtok(argv[count], "\n");
        count++;
    }

    pid = fork();

    if(pid < 0) {
        fprintf(stderr, "ERROR: failed to fork child process");
        exit(1);
    } else if(pid == 0) {
        //child process
        if(execvp(argv[0], argv) < 0) {
            fprintf(stderr, "ERROR: exec failed, file may not exist\n");
            exit(1);
        }
    } else {
        //parent process
        waitpid(pid, &status, WUNTRACED);
    }
}

void run_cmd() {

    char *command = argv[0];

    char *current_dir = (char *) malloc(514);
    getcwd(current_dir, 514);

    //if asking for current working directory, print it
    if (strcmp(command, "pwd\n") == 0) {
        see_directory(current_dir);
    } else if (strcmp(command, "cd\n") == 0) {
        go_home();
    } else if (strcmp(command, "cd") == 0) {
        go_to_dir(current_dir, argv[1]);
    } else if(strcmp(command, "unsetenv") == 0) {
        delete_var(argv[1]);
    } else if(strcmp(command, "setenv") == 0) {
        make_var(argv[1], argv[2]);
    } else {

        //execute file
        execute();
    }

    free(current_dir);

}

void make_var(char *var, char *value){

    int success;

    //get rid of newline characters, just in case
    strtok(var, "\n");
    strtok(value, "\n");

    char *old_val = getenv(var);

    if(value == NULL || strcmp(value, "\n") == 0) {
        success = setenv(var, "", 1);
    } else {
        success = setenv(var, value, 1);
    }

    if(success == -1) {
        fprintf(stderr, "ERROR: failed to change %s", var);
    }
}

void delete_var(char *var) {
    if(var == NULL) {
        fprintf(stderr, "ERROR: no variable given to unset");
        return;
    }

    strtok(var, "\n");

    int success = unsetenv(var);
    if(success == -1) {
        fprintf(stderr, "ERROR: failed to delete specified variable: %s", var);
    }
}

void go_to_dir(char *current_dir, char *dir){

    char new_dir[strlen(dir)+1];
    strcpy(new_dir, "/");
    strcat(new_dir,dir);

    size_t length = strlen(new_dir) - 1;
    if (*new_dir && new_dir[length] == '\n') {
        new_dir[length] = '\0';
    }

    strcat(current_dir, new_dir);
    int success = chdir(current_dir);

    if (success == -1) {
        fprintf(stderr, "ERROR: directory does not exist\n");
    } else {
        see_directory(current_dir);
    }
}

void go_home() {

    char *home = getenv("HOME");

    chdir(home);
    see_directory(home);

}

void see_directory(char *current_dir) {
    printf("%s\n", current_dir);
}

//gives user prompt to input arguments and accepts them
void queryUser()
{
    char input[514];
    printf("? ");

    if(fgets(input,514,stdin) == NULL){
        printf("Ctrl-D: exiting process\n");
        exit(0);
    }

    //check if over 512
    if (strchr(input, '\n') == NULL) {
        fprintf(stderr, "ERROR: Input too long!\n");
        //get rid of extra input
        int c;
        while((c = getc(stdin)) != '\n' && c != EOF);
        return;
    }

    //tokenize into arguments
    get_args(input);
}

//splits arguments by whitespace
void get_args(char *input)
{
    char *token = NULL;

    //get first part of input
    token = strtok(input, " ");
    argv[argc] = token;

    //check for exit command
    if(strcmp(token, "exit\n") == 0) {
        printf("exit: exiting process\n");
        exit(0);
    }

    //capture rest of input
    for(;;) {

        token = strtok(NULL, " ");

        argc++;

        if (token == NULL) {
            argv[argc] = NULL;
            break;
        }

        argv[argc] = token;

        //check for exit command
        if(strcmp(token, "exit") == 0 || strcmp(token, "exit\n") == 0) {
            printf("exit: exiting process\n");
            exit(0);
        }
    }

    //run commands
    run_cmd();
}
