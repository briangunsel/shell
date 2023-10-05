#define LINE_MAX 4096
#define COMMAND_LENGTH 1024
#define MAX_ARGUMENTS 15

#include <stdio.h>

char **parse(char *line, char *delim);
int find_special (char *args[], char *special);
FILE *getInput(int argc, char *argv[]);

void exit_shell();
void help();
char *pwd();
void change_dir(char *path, int num_args);
void executable(char *path_to_program, char *args[], int in, int out, int redirection, int bg);
int exists(const char *path);
void piping_exec(char *input[], int n, int bg);
char *removeSpaces(char *str);
void wait_();

