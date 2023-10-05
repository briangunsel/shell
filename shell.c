#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void exit_shell();
void help();
char *pwd();
void change_dir(char *path, int num_args);
void executable(char *path_to_program, char *args[], int in, int out, int redirection, int bg);
int exists(const char *path);
char *get_path(char *command);
void piping_exec(char *input[], int n, int bg);
char *removeSpaces(char *s);
void wait_();

int main(int argc, char **argv) {
    char *prompt = "TUShell $ > ";
    char command[COMMAND_LENGTH];
    char *arguments[MAX_ARGUMENTS];
    int input_fd;
    int output_fd;
    char *pipe_cmds[MAX_ARGUMENTS];

    while(1) {
        printf("%s", prompt);
        fgets(command, COMMAND_LENGTH, stdin);
        command[strcspn(command, "\n")] = 0;
        int redirection = 0; // helper variable for lines 67-86
        int piping = 0;
        int background = 0;
        int num_args = 0;
        int num_pipes = 0;

        if(strchr(command, '&') != NULL) {
            background = 1;
            command[strlen(command) - 1] = '\0'; // remove the '&' from the end of the command
        }

        if(strchr(command, '>') != NULL || strchr(command, '<') != NULL) {
            redirection = 1;
        }

        if(strchr(command, '|') == NULL) {
            char *token = strtok(command, " ");
            while (token != NULL && num_args < MAX_ARGUMENTS - 1) {
                arguments[num_args++] = token;
                token = strtok(NULL, " ");
            }

            arguments[num_args] = NULL;

        } else { // needs to be piped.
            piping = 1;
            char *token2 = strtok(command, "|");
            while(token2 != NULL) {
                pipe_cmds[num_pipes++] = token2;
                token2 = strtok(NULL, "|");
            }

            pipe_cmds[num_pipes] = NULL;
        }

        input_fd = STDIN_FILENO;
        output_fd = STDOUT_FILENO;

        // check for redirection
        for(int i = 0; i < num_args; i++) {
            if(strcmp(arguments[i], "<") == 0) {
                input_fd = open(arguments[i+1], O_RDONLY);
                if(input_fd < 0) {
                    perror("open");
                }

                arguments[i] = NULL;

            } else if(strcmp(arguments[i], ">") == 0) {
                output_fd = open(arguments[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if(output_fd < 0) {
                    perror("open");
                }

                arguments[i] = NULL;
            }
        }

        if(redirection == 1 && piping == 1) {
            perror("redirection and piping not supported.\n");
            continue;
        }

        if(strcmp(arguments[0], "exit") == 0 && redirection != 1 && piping != 1) { // exit shell
            exit_shell();
            return 0;

        } else if(strcmp(arguments[0], "help") == 0 && redirection != 1 && piping != 1) { // help
            help();

        } else if(strcmp(arguments[0], "pwd") == 0 && redirection != 1 && piping != 1) { // print directory
            char *wd; // wd = working directory
            wd = pwd();
            printf("%s\n", wd);
            free(wd);

        } else if(strcmp(arguments[0], "cd") == 0 && redirection != 1 && piping != 1) { // change directory
            change_dir(arguments[1], num_args);

        } else if(strcmp(arguments[0], "wait") == 0 && redirection != 1 && piping != 1) { // wait
            wait_();

        } else {
            if(piping == 0) {
                executable(arguments[0], arguments, input_fd, output_fd, redirection, background);
            } else if(piping == 1) {
                piping_exec(pipe_cmds, num_pipes, background);
            }
        }
    }
}

void executable(char *path_to_program, char *args[], int in, int out, int redirection, int bg) { // execute non built-ins
    pid_t pid = fork();

    if(pid == -1) { // error
        perror("fork failed.\n");

    } else if(pid == 0) { // child
        if(redirection == 1) {
            if (in != STDIN_FILENO) {
                dup2(in, STDIN_FILENO);
                close(in);
            }
            if (out != STDOUT_FILENO) {
                dup2(out, STDOUT_FILENO);
                close(out);
            }
        }

        if(strchr(path_to_program, '/') != NULL) { // contains '/'
            execv(path_to_program, args);
        } else {
            char *path = getenv("PATH");
            char *save_ptr = NULL;
            char path_buf[256];
            strcpy(path_buf, path);

            char *tok = strtok_r(path_buf, ":", &save_ptr);
            while(tok != NULL) {
                char program[256];
                snprintf(program, 256, "%s/%s", tok, path_to_program);
                execv(program, args);
                tok = strtok_r(NULL, ":", &save_ptr);
            }

            perror("command not found.\n");
        }

        exit(0);

    } else { // parent
        if(bg == 0) {
            int status;
            waitpid(pid, &status, 0);
        } else if(bg == 1) {
            int status;
            waitpid(-1, &status, WNOHANG);
        }
    }
}

void piping_exec(char *input[], int n, int bg) {
    int pipes[n - 1][2];

    // create pipes
    for(int i = 0; i < n - 1; i++) {
        if(pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for(int i = 0; i < n; i++) {
        pid_t pid = fork();

        if(pid == -1) {
            perror("fork failed.\n");
            exit(1);

        } else if(pid == 0) { // child
            // If this is not the first command, read from the previous pipe
            if(i > 0) {
                if(dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }

            // If this is not the last command, write to the next pipe
            if(i < n - 1) {
                if(dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            char *arguments[MAX_ARGUMENTS];
            char *token = strtok(input[i], " ");
            int j = 0;
            while(token != NULL && j < MAX_ARGUMENTS - 1) {
                arguments[j++] = token;
                token = strtok(NULL, " ");
            }
            arguments[j] = NULL;

            if(execvp(arguments[0], arguments) == -1) {
                perror("execvp");
                exit(1);
            }

        } else { // parent
            // Close the read and write ends of the previous pipe
            if(i > 0) {
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }
        }
    }

    // Wait for all child processes to finish
    for(int i = 0; i < n; i++) {
        if(bg == 0) {
            wait(NULL);
        } else if(bg == 1) {
            int status;
            waitpid(-1, &status, WNOHANG);
        }
    }
}

int exists(const char *path) { // helper function for executable() to see if program exists
    struct stat st;

    if(stat(path, &st) < 0) {
        return -1;
    }

    return S_ISREG(st.st_mode);
}

char *removeSpaces(char *str) {
    int count = 0;
    for (int i = 0; str[i]; i++)
        if (str[i] != ' ')
            str[count++] = str[i];

    str[count] = '\0';

    return str;
}
