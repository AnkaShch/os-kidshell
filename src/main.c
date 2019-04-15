#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <wordexp.h>
#include <string.h>

#include "exit_messages.h"

#define MAX_LEN 10000

int main(int argc, char *argv[], char *envp[]) {
    char command[MAX_LEN];
    pid_t child_pid;
    char exit_status_message[MAX_LEN];
    int wstatus;
    int i;
    wordexp_t p;


    init_messages();


    printf(">> ");

    while (fgets(command, sizeof(command), stdin)) {

        /* exit command */
        if (!strcmp(command, "exit\n"))
            return EXIT_SUCCESS;

        command[strlen(command) - 1] = '\0';
        i = 0;
        while (i < strlen(command) && command[i] != '=') {
            i++;
        }
        if (i < strlen(command)) {
            command[i] = ' ';
        }
        
        wordexp(command, &p, 0);

        if (!strcmp(p.we_wordv[0], "export") && p.we_wordc == 3) {
            printf("Set variable %s to \"%s\"\n", p.we_wordv[1], p.we_wordv[2]);
            setenv(p.we_wordv[1], p.we_wordv[2], true);
            printf(">> ");
            continue;
        } 
        
        if (!strcmp(p.we_wordv[0], "unset") && p.we_wordc == 2) {
            unsetenv(p.we_wordv[1]);
            printf("Unset variable %s\n", p.we_wordv[1]);
            printf(">> ");
            continue;
        }

        child_pid = fork();
        switch (child_pid) {
        case -1:
            perror("forking error");
            exit(EXIT_FAILURE);
            break;

        case 0:
            /* Child */
            (void)execve(p.we_wordv[0], p.we_wordv, envp);
            printf("Error on program start\n");
            exit(EXIT_FAILURE);
            break;
            /* End child */

        default:
            /* Parent */
            do {
                /* man 2 wait */
                if (waitpid(child_pid, &wstatus, WUNTRACED) == -1) {
                    perror("waitpid() failed");
                    exit(EXIT_FAILURE);
                }

                if (WIFEXITED(wstatus)) {
                    /* Child terminated normally */
                    get_exit_message(exit_status_message, WEXITSTATUS(wstatus));
                    puts(exit_status_message);
                } else if (WIFSIGNALED(wstatus)) {
                    /* Child process was terminated by a signal */
                    get_exit_message(exit_status_message, 128 + WTERMSIG(wstatus));
                    puts(exit_status_message);
                } else if (WIFSTOPPED(wstatus)) {
                    printf("Stopped by signal %d\n", WSTOPSIG(wstatus));
                }
            } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));

            wordfree(&p);
            printf("\n");
            break;
            /* End parent */
        }
        printf(">> ");
    }

    return EXIT_SUCCESS;
}
