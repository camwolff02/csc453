/**
 * SLOsh - San Luis Obispo Shell
 * CSC 453 - Operating Systems
 * 
 * TODO: Complete the implementation according to the comments
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
 
/* Define PATH_MAX if it's not available */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64

/* Global variable for signal handling */
volatile sig_atomic_t child_running = 0;

/* Forward declarations */
void display_prompt(void);

/**
* Signal handler for SIGINT (Ctrl+C)
* 
* TODO: Implement the signal handler to:
* 1. Print a newline
* 2. If no child is running, display a prompt
* 3. Make sure the shell doesn't exit when SIGINT is received
*/
void sigint_handler() {
	/* TODO NOTE: we shouldn't use printf in a signal handler, we need to set a flag */
	printf("\n");

	if (!child_running) {
		printf("No child running!\n");
		display_prompt();
	} else {
		kill(child_running, SIGALRM);
	}

 }
 
/**
* Display the command prompt with current directory
*/
void display_prompt(void) {
	char cwd[PATH_MAX];
     
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
    	printf("%s> ", cwd);
    } else {
        perror("getcwd");
        printf("SLOsh> ");
    }
    fflush(stdout);
}
 
/**
* Parse the input line into command arguments
* 
* Parse the input string into tokens and store in args array
* 
* @param input The input string to parse
* @param args Array to store parsed arguments
* @return Number of arguments parsed
*/
int parse_input(char *input, char **args) {
	const int LEN = strlen(input);
	int arg_idx = 0;
	char buf[MAX_INPUT_SIZE];

	// If there is no input, we've read no arguments
	if (LEN == 0) {
		return 0;
 	}

	// Loop through each character in input
	for (int input_idx = 0, buf_idx = 0; input_idx < LEN; input_idx++) {
		// If there's a space, its time to add the whole argument
		if (input[input_idx] == ' ' || input[input_idx] == '\n') {  
			// null terminate the arg
			buf[buf_idx++] = '\0';  
			// allocate a cstring to hold the arg
			args[arg_idx] = (char *)malloc(sizeof(char) * buf_idx);
			// copy the arg to its cstring
			strncpy(args[arg_idx], buf, buf_idx);
			// move to the next arg and reset the buffer
			arg_idx++;
			buf_idx = 0;
		} else {
	 		// If there's not a space, add this character to the current buffer
			buf[buf_idx++] = input[input_idx];
		}

	}
	return arg_idx;
}
 
/**
* Execute the given command with its arguments
* 
* TODO: Implement command execution with support for:
* 1. Basic command execution
* 2. Pipes (|)
* 3. Output redirection (> and >>)
* 
* @param args Array of command arguments (NULL-terminated)
*/
void execute_command(char **args) {
	/* Hints:
	 * 1. Fork a child process
	 * 2. In the child, reset signal handling and execute the command
	 * 3. In the parent, wait for the child and handle its exit status
	 * 4. TODO For pipes, create two child processes connected by a pipe
	 * 5. TODO For redirection, use open() and dup2() to redirect stdout
	 */

	// Fork a child process
	int status;
	child_running = fork();

	if (child_running) {  // parent
		// Wait for the child and handle its exit status
		waitpid(child_running, &status, 0);
		child_running = 0;
		// TODO handle exit status
	} else {  // child
		// Reset signal handling and execute the command
		signal(SIGINT, SIG_DFL); 
		execvp(args[0], args);  
		exit(EXIT_FAILURE);
		// TODO ERROR: if I do "ls" then "ls -a", ls doesn't work anymore
		// pwd also adds weird other part
	}
}
 
/**
* Check for and handle built-in commands
* 
* TODO: Implement support for built-in commands:
* - exit: Exit the shell
* - cd: Change directory
* 
* @param args Array of command arguments (NULL-terminated)
* @return 0 to exit shell, 1 to continue, -1 if not a built-in command
*/
int handle_builtin(char **args) {
	if (!strcmp(args[0], "cd")) {
		// make sure they passed a directory as an argument
		if (args[1] != NULL) {
			struct stat path_stat;
			stat(args[1], &path_stat);
			if (S_ISDIR(path_stat.st_mode)) {
				chdir(args[1]);
			} else {
				printf("ERROR: \"%s\" is not a directory\n", args[1]);
			}
		} else {
			// handle empty cd command by sending user to home directory
			struct passwd *pw = getpwuid(getuid());
			chdir(pw->pw_dir);
		}
		return 1;
	} else if (!strcmp(args[0], "exit")) {
		return 0;
	}

    return -1;  /* Not a builtin command */
 }
 
int main(void) {
	char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
	int n_args = 0;
    int status = 1;
    int builtin_result;
	struct sigaction sigint_action;
    
    /* Set up signal handling for SIGINT (Ctrl+C) TODO check */
	sigint_action.sa_handler = sigint_handler;
 	sigint_action.sa_flags = SA_RESTART;
 	sigemptyset(&sigint_action.sa_mask);
	sigaction(SIGINT, &sigint_action, NULL);

    while (status) {
		/* Free args */
		for (int i = 0; i < n_args; i++) {
			free(args[i]);
			args[i] = NULL;
		}

        display_prompt();
         
        /* Read input and handle signal interruption */
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            /* TODO: Handle EOF and signal interruption */
            break;
        }
         
        /* Parse input */
        n_args = parse_input(input, args);
         
        /* Handle empty command */
        if (args[0] == NULL) {
			printf("No command passed\n");
            continue;
        }
         
        /* Check for built-in commands */
        builtin_result = handle_builtin(args);
        if (builtin_result >= 0) {
            status = builtin_result;
            continue;
        }
         
        /* Execute external command */
        execute_command(args);
     }

	 for (int i = 0; i < n_args; i++) {
		 free(args[i]);
		 args[i] = NULL;
	 }

     printf("SLOsh exiting...\n");
     return EXIT_SUCCESS;
}
