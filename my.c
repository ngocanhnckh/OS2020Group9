#define _POSIX_SOURCE
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH (70)
#define MAX_NUM_ARGS (5)

void ignoreSignal(int);
void resetSignal(int);
void readCommand(char*, int);
void parseCommand(char*, char**, char**);
void cd(char*);
void goHome();
void quit(int);
void runCommand(char*, char*[]);
unsigned long getTimestamp();

int main(int argc, char* argv[]) {

  char buffer[MAX_COMMAND_LENGTH];

  char* command;

  char* args[MAX_NUM_ARGS + 2];

  pid_t child_pid;
  args[MAX_NUM_ARGS + 1] = NULL;

  ignoreSignal(SIGINT);
  ignoreSignal(SIGTERM);

  while(1) {
    while((child_pid = waitpid(-1, NULL, WNOHANG)) > 0) {
      printf("Background process %d terminated\n", child_pid);
    }

    printf("minishell>");
    readCommand(buffer, MAX_COMMAND_LENGTH);
    parseCommand(buffer, &command, args);
    if(command == NULL) {
      continue;
    }

    if(strcmp(command, "exit") == 0) {
      quit(0);
    } else if(strcmp(command, "cd") == 0) {
      cd(args[1]);
    } else {
      runCommand(command, args);
    }
  }

  exit(1);
}

void ignoreSignal(int sig) {
  if(signal(sig, SIG_IGN) == SIG_ERR) {
    printf("Failed to install signal handler for signal %d\n", sig);
    exit(1);
  }
}

void resetSignal(int sig) {
  if(signal(sig, SIG_DFL) == SIG_ERR) {
    printf("Failed to reset signal handler for signal %d\n", sig);
    exit(1);
  }
}


void readCommand(char* buffer, int max_size) {
  size_t ln;
  char* return_value = fgets(buffer, max_size, stdin);
  if(return_value == NULL) {
    quit(1);
  }

  ln = strlen(buffer) - 1;
  if(buffer[ln] == '\n') {
    buffer[ln] = '\0';
  }
}

void parseCommand(char* buffer, char** command, char** args) {
  int i;
  *command = strtok(buffer, " ");
  args[0] = *command;
  for(i = 1; i < MAX_NUM_ARGS + 1; i++) {
    args[i] = strtok(NULL, " ");
  }
}

void cd(char* directory) {
  if(directory == NULL) {
    goHome();
    return;
  }

  if(chdir(directory) == -1) {
    fprintf(stderr, "Unable to go to directory %s. Attempting to go to HOME directory\n", directory);
    goHome();
  }
}

void goHome() {
  char* home = getenv("HOME");
  if(chdir(home) == -1) {
    fprintf(stderr, "Unable to go to HOME directory. Possibly not set.\n");
  }
}

void quit(int status) {
    kill(0, SIGTERM);
    exit(status);
  }

void runCommand(char* command, char* args[]) {
  int background = 0;
  int i;
  unsigned long startTime;
  pid_t pid = fork();
  if(pid == -1) {
    fprintf(stderr, "Unable to fork.\n");
    quit(1);
  }

  for(i = MAX_NUM_ARGS; i >= 0; i--) {
    if(args[i] != NULL) {
      if(strcmp(args[i], "&") == 0) {
        background = 1;
        args[i] = NULL;
      }
      break;
    }
  }

  if(pid == 0) {
    resetSignal(SIGINT);
    resetSignal(SIGTERM);
    execvp(command, args);
    fprintf(stderr, "Unable to start command: %s\n", command);
    quit(1);
  } else if(pid > 0) {
    startTime = getTimestamp();
    printf("Started %s process with pid %d\n", background ? "background" : "foreground", pid);
    if(background) {
      return;
    }

    while(waitpid(pid, NULL, 0) == -1);
    printf("Foreground process %d ended\n", pid);
    printf("Wallclock time: %.3f\n", (getTimestamp() - startTime) / 1000.0);
  }
}

unsigned long getTimestamp() {
  struct timeval tv;
  if(gettimeofday(&tv,NULL) == -1) {
    fprintf(stderr, "Unable to get current time stamp.\n");
    return 0;
  }
  return 1000000 * tv.tv_sec + tv.tv_usec;
}
