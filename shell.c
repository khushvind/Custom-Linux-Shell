#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 2048
#define MAX_ARGS 64
#define MAX_HISTORY 100
#define PATH_MAX 4096

// History contains a list of inputs(strings) that were given to the shell
char *history[MAX_HISTORY];
int history_cnt = 0;

// Adds the command to history
void add_history(char *command) {
    if (history_cnt < MAX_HISTORY) {
        history[history_cnt++] = strdup(command);
    } else {
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++) 
            history[i-1] = history[i];
        history[MAX_HISTORY-1] = strdup(command);
    }
    return;
}

// Displays the history
void disp_history() {
    for (int i =0; i <history_cnt; i++) 
        printf("%s\n", history[i]);
    return;
}

void change_dir(char *path) {
    // These handle different cases for cd command, like cd, cd ~, cd -, cd path
    if (path == NULL || strcmp(path, "~") == 0){
        // for handling, cd , and cd -
        if (chdir(getenv("HOME")) != 0) {
            fprintf(stderr, "Invalid Command\n");
            return;
        }
    } else if (strcmp(path, "-") == 0){
        // for handling, cd -
        char *prev_dir = getenv("OLDPWD");
        if (prev_dir == NULL){
            fprintf(stderr, "Invalid Command\n");
            return;
        }
        if (chdir(prev_dir) != 0){
            fprintf(stderr, "Invalid Command\n");
            return;
        }
    } else{
        // for handling, cd path
        if (chdir(path) != 0){
            // Path might be quoted, this checks if the path is quoted
            char *q_path = malloc(strlen(path)+1);
            strcpy(q_path, path);
            if (q_path[0] == '\'' &&  q_path[strlen(q_path) - 1] == '\'') {
                // Remove single quotes
                q_path[strlen(q_path) - 1] = '\0';
                q_path++;
                if (chdir(q_path) != 0){
                    fprintf(stderr, "Invalid Command\n");
                    return;
                }
            } else if (q_path[0] == '"' && q_path[strlen(q_path) - 1] == '"'){
                // Remove double quotes
                q_path[strlen(q_path) - 1] = '\0';
                q_path++;
                if (chdir(q_path) != 0){
                    fprintf(stderr, "Invalid Command\n");
                    return;
                }
            } else{
                fprintf(stderr, "Invalid Command\n");
                return;
            }
        }
    }

    setenv("OLDPWD", getenv("PWD"), 1);
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) setenv("PWD", cwd, 1);
    return;
}

// Executes the command
void execute_command(char **args) {
    pid_t pid = fork();
    if (pid == -1){
        fprintf(stderr, "Invalid Command\n");
        return;
    } else if (pid == 0){
        // This handles the child process
        if (strcmp(args[0], "ls") == 0) {
            // These check if the argument is of form 'ls -1 path' or 'ls path'
            // Check if a path argument is provided
            if (args[1] != NULL && strcmp(args[1], "-1") == 0) {
                // Check if a path argument is provided after '-1'
                if (args[2] != NULL) {
                    // Check if the path exists
                    if (access(args[2], F_OK) != 0) {
                        fprintf(stderr, "Invalid Command\n");
                        return;
                    }
                }
            } else if (args[1] != NULL) {
                // Check if the path exists
                if (access(args[1], F_OK) != 0)  {
                    // Path does not exist or other error occurred
                    fprintf(stderr, "Invalid Command\n");
                    return;
                }
            }
        } else if (strcmp(args[0], "cat") == 0) {
            // These check if the argument is of form 'cat path'
            // Check if a path argument is provided
            if (args[1] != NULL) {
                // Check if the path exists
                if (access(args[1], F_OK) != 0)  {
                    // Path does not exist or other error occurred
                    fprintf(stderr, "Invalid Command\n");
                    return;
                }
            }
        } 

        if (execvp(args[0], args) == -1){
            fprintf(stderr, "Invalid Command\n");
            return;
        }
    } else{
        // This handles the parent process
        int status;
        waitpid(pid, &status, 0);
        if (strcmp(args[0], "cat") == 0){
            int is_piped = 0;
            for (int i = 0; args[i] != NULL; i++){
                if (strcmp(args[i], "|") == 0){
                    is_piped = 1;
                    break;
                }
            }
            // This case is to make sure that 'MTL458 >' starts from a new line.
            if (!is_piped) printf("\n");
        }
    }
    return ;
}

// Executes piped commands
void execute_piped_commands(char **cmd1, char **cmd2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Invalid Command\n");
        return;
    }
    pid_t pid1 = fork();
    if (pid1 == -1) {
        fprintf(stderr, "Invalid Command\n");
        return;
    } else if (pid1 == 0) {
        // First child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        if (execvp(cmd1[0], cmd1) == -1) {
            fprintf(stderr, "Invalid Command\n");
            return;
        }
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        fprintf(stderr, "Invalid Command\n");
        return;
    } else if (pid2 == 0) {
        // Second child process
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        if (execvp(cmd2[0], cmd2) == -1) {
            fprintf(stderr, "Invalid Command\n");
            return;
        }
    }
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    return;
}

// Checks if the command was piped
int check_if_piped(char **args, int i){
    int pipe_index = -1;
        for (int j = 0; j < i; j++) {
            if (strcmp(args[j], "|") == 0){
                pipe_index = j;
                break;
            }
        }
    return pipe_index;
}

// Executes the commands other than history, and exit
void execute(char *input){
    char *args[MAX_ARGS];
    char *tk;
    int i;
    i = 0;
    tk = strtok(input, " ");
    while (tk != NULL && i < MAX_ARGS - 1){
        args[i] = tk;
        i++;
        tk = strtok(NULL, " ");
    }
    args[i] = NULL;
    if (i > 0) {
        if (strcmp(args[0], "cd") == 0){
            change_dir(args[1]);
        } else if ((strcmp(args[0], "pipe") == 0 || strcmp(args[0], "grep") == 0) && args[1] == NULL){
            // This handles these specific exceptions.
            fprintf(stderr, "Invalid Command\n");
        } else {
            int pipe_index = check_if_piped(args, i);
            if (pipe_index != -1) {
                args[pipe_index] = NULL;
                execute_piped_commands(args, &args[pipe_index + 1]);
            } else {
                execute_command(args);
            }
        }
    }
    return;
}

int main() {
    char input[MAX_INPUT_SIZE];

    while (1) {
        printf("MTL458 > ");
        // Takes the input
        if (fgets(input, sizeof(input), stdin) == NULL) break;
    
        input[strcspn(input, "\n")] = '\0';
        add_history(input);
        if (strcmp(input, "exit") == 0){
            break;
        } else if (strcmp(input, "history") == 0){
            disp_history();
        } else {
            execute(input);
        }
    }
    return 0;
}