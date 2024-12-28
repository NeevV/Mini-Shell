#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_ALIASES 100
#define HISTORY_SIZE 100
#define MAX_CMD_LENGTH 1024

typedef struct {
    char name[50];
    char command[256];
} Alias;

typedef struct Job {
    int jobId;
    pid_t pid;
    char command[256];
    struct Job *next;
} Job;

Alias aliases[MAX_ALIASES];
int aliasCount = 0;

char *history[HISTORY_SIZE];
int historyCount = 0;

Job *jobList = NULL;
int jobCount = 0;

//creating function Prototypes
void addJob(pid_t pid, char *command);
void removeJob(pid_t pid);
 void listJobs();
void foregroundJob(int jobId);
void backgroundJob(int jobId);
void handleSignals(int signal);
void executeCommand(char **args, int background);
void parseAndExecute(char *input);
void handleRedirectionAndPipes(char **args);
void addHistory(char *command);
void printHistory();
void addAlias(char *name, char *command);
void removeAlias(char *name);
void listAliases();
int expandAlias(char **args);

//this function adds  job to the list
void addJob(pid_t pid,char *command){
    Job *newJob= malloc(sizeof(Job));
    newJob->jobId = ++jobCount;
    newJob->pid=pid;
    strncpy(newJob->command, command, 256);
     newJob->next = jobList;
    jobList =newJob;
    }

// Remove a job from the list by PID
void removeJob(pid_t pid){
    Job *current =jobList, *prev =NULL;
    while (current!= NULL){
        if (current->pid == pid) {
            if(prev==NULL) {
                jobList= current->next;
            } else {
                prev->next= current->next;
            }
            free(current);
            return;
        }
        prev= current;
        current = current->next;
    }
}

// this lists all active jobs
void listJobs() {
    Job *current =jobList;
    while(current!= NULL) {
        printf("[%d] %d %s\n", current->jobId, current->pid, current->command);
        current=current->next;
    }
}

//bring a job to the foreground
void foregroundJob(int jobId) {
    Job *current= jobList;
    while (current!= NULL) {
        if(current->jobId ==jobId) {
            int status;
            printf("Bringing job [%d] (%s) to foreground\n",current->jobId, current->command);
             kill(current->pid, SIGCONT);
            waitpid(current->pid, &status, 0);
            removeJob(current->pid);
            return;}
        current = current->next;
    }
    printf("fg: %d: no such job\n", jobId);
}

 //resume a job in the background
void backgroundJob(int jobId) {
    Job *current = jobList;
    while (current != NULL) {
        if (current->jobId == jobId) {
            printf("Resuming job [%d] (%s) in background\n", current->jobId, current->command);
            kill(current->pid, SIGCONT);
            return;
        }
        current =current->next;
    }
    printf("bg: %d: no such job\n",jobId);
}

//signal handler for job processes
void handleSignals(int signal) {
    if (signal== SIGINT) {
        printf("\nCaught SIGINT (CTRL+C). Type 'exit' to quit.\n");
    } else if (signal== SIGTSTP) {
        printf("\nCaught SIGTSTP (CTRL+Z). Use 'fg' to resume.\n");}
}

//add a command to history
void addHistory(char *command) {
    if (historyCount< HISTORY_SIZE) {
        history[historyCount++]= strdup(command);
    }
}

// print command history
void printHistory() {
    for(int i = 0; i<historyCount; i++) {
        printf("%d %s\n", i+1, history[i]);}
}

// add or update an alias
void addAlias(char *name, char *command) {
    for (int i = 0; i<aliasCount; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
             strncpy(aliases[i].command, command, 256);
            return;
        }
    }
    strncpy(aliases[aliasCount].name, name, 50);
     strncpy(aliases[aliasCount].command, command, 256);
    aliasCount++;
}

// removes an alias
void removeAlias(char *name) {
    for(int i = 0; i<aliasCount; i++) {
        if(strcmp(aliases[i].name, name) ==0) {
            for (int j = i; j<aliasCount -1; j++) {
                aliases[j] =aliases[j + 1];
            }
             aliasCount--;
            return;
        }
    }
    printf("unalias: %s: not found\n", name);
}

// printsall aliases
void listAliases(){
    for(int i = 0; i <aliasCount; i++){
        printf("alias %s='%s'\n",aliases[i].name, aliases[i].command);}
}

// expand alias in input
int expandAlias(char **args) {
    for(int i=0; i<aliasCount; i++) {
        if (strcmp(aliases[i].name,args[0]) == 0) {
             char expanded[1024];
            snprintf(expanded,sizeof(expanded), "%s %s",
                     aliases[i].command,args[1] ? args[1] : "");
            parseAndExecute(expanded);
            return 1;}
    }
    return 0;
}

// Parse and execute the input
void parseAndExecute(char *input) {
    char *args[256];
     int background = 0;

    // Check for background execution
    if(strchr(input, '&')) {
        background =1;
        *strchr(input, '&') = '\0';  // Remove the '&' character from the input
    }

    // Tokenize input
    int i = 0;
    args[i] = strtok(input, " \t\n");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }

    // Check for alias expansion
    if (expandAlias(args)) {
        return;
    }

    // Built-in commands
    if (args[0] == NULL) {
        return;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || chdir(args[1]) != 0) {
            perror("cd");
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
    } else if (strcmp(args[0], "alias") == 0) {
        if (args[1] == NULL) {
            listAliases();
        } else {
            char *equalsSign = strchr(args[1], '=');
            if (equalsSign) {
                *equalsSign = '\0';
                char *name = args[1];
                char *command = equalsSign + 1;
                if (command[0] == '\'' || command[0] == '"') {
                    char quote = command[0];
                    char *end = strrchr(command, quote);
                    if (end) {
                        *end = '\0';
                        command++;
                    }
                }
                addAlias(name, command);
            } else {
                printf("alias: Invalid syntax. Use alias name='command'\n");
            }
        }
    } else if (strcmp(args[0], "unalias") == 0) {
        if (args[1] != NULL) {
            removeAlias(args[1]);
        } else {
            printf("unalias: usage: unalias name\n");
        }
    } else if (strcmp(args[0], "jobs") == 0) {
        listJobs();
    } else if (strcmp(args[0], "fg") == 0) {
        if (args[1] != NULL) {
            foregroundJob(atoi(args[1]));
        } else {
            printf("fg: missing job ID\n");
        }
    } else if (strcmp(args[0], "bg") == 0) {
        if (args[1] != NULL) {
            backgroundJob(atoi(args[1]));
        } else {
            printf("bg: missing job ID\n");
        }
    } else if (strcmp(args[0], "history") == 0) {
        printHistory();
    } else {
        executeCommand(args, background);
    }

    addHistory(input);
}

// Execute command with optional background support
void executeCommand(char **args, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        handleRedirectionAndPipes(args);  // Handle pipes and redirection before execvp

        // Debugging line to print the command being executed
        printf("Executing command: %s\n", args[0]);

        if (execvp(args[0], args) == -1) {
            if (errno == ENOENT) {
                // Command not found
                fprintf(stderr, "mini-shell: %s: command not found\n", args[0]);
            } else {
                // Other errors (permission denied, etc.)
                perror("execvp");
            }
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        if (background) {
            addJob(pid, args[0]);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

// Handle redirection and pipes
void handleRedirectionAndPipes(char **args) {
    int fd[2], inFd = 0, i = 0;

    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            pipe(fd);
            if (fork() == 0) {
                dup2(inFd, 0);
                dup2(fd[1], 1);
                close(fd[0]);
                args[i] = NULL;
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                wait(NULL);
                close(fd[1]);
                inFd = fd[0];
                args += i + 1;
                i = 0;
            }
        } else if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            dup2(fd, 1);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            dup2(fd, 0);
            close(fd);
            args[i] = NULL;
        } else {
            i++;
        }
    }

    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
}

// Main function
int main() {
    char input[MAX_CMD_LENGTH];

    signal(SIGINT, handleSignals);
    signal(SIGTSTP, handleSignals);

    while (1) {
        printf("mini-shell> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        parseAndExecute(input);
    }

    return 0;
}
