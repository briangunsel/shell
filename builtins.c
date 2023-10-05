#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include "helpers.h"
#include <string.h>
#include <unistd.h>

void exit_shell() {
    printf("%s", "exiting shell...\n\n");
    exit(EXIT_SUCCESS);
}

void wait_() {
    while (1) {
        pid_t pid = waitpid(-1, NULL, WNOHANG);

        if (pid == 0) {
            continue;
        } else if (pid == -1) {
            break; // jobs have finished
        }

        printf("Background job %d has terminated\n", pid);
    }

    return;
}

void help() { // display help.txt
    FILE *fp_help = fopen("help.txt", "r");
    if(fp_help == NULL) {
        printf("unable to display help command.\n");
        return;
    }

    char c = fgetc(fp_help);
    while(c != EOF) {
        printf("%c", c);
        c = fgetc(fp_help);
    }

    fclose(fp_help);
    puts("\n");
}

char *pwd() {
    char *path = NULL;
    char *buf;
    long size;

    size = pathconf(".", _PC_PATH_MAX);

    if((buf = (char *)malloc((size_t)size)) != NULL) {
        path = getcwd(buf, (size_t)size);
    }

    return path;
}

void change_dir(char *path, int num_args) {
    if(num_args == 1) {
        printf("%s", "command 'cd' requires 1 argument. see 'help' for more info.\n");
    } else if(num_args == 2) {
        if(chdir(path) != 0) {
            perror("failed to execute 'cd'\n");
        }
    }
}