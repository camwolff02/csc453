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
	 * 5. For redirection, use open() and dup2() to redirect stdout
	 */
	int status;
	int append = 0;
	int redirect_idx = 0;  
	int pipe_idxs[MAX_INPUT_SIZE / 2];

	// First, check for special conditions and valid commands
	for (int idx = 0, pipe_idx = 0; args[idx] != NULL; idx++) {
		if (args[idx][0] == '>') {
			if (idx == 0) {
				printf("ERROR: Redirect used with no program specified\n");
			} else if (args[idx+1] == NULL) {
				printf("ERROR: Redirect used with no file specified\n");
				return;
			} else if (!access(args[idx+1], F_OK) && access(args[idx+1], W_OK)) {
				// If the file exists and we can't write to it, we can't redirect 
				printf("ERROR: Cannot write to specified file\n");
				return;
			} else {
				// if we're here, we know it's safe to write to the file
				redirect_idx = idx;
				append = args[idx][1] == '>';

				// Insert new end to args
				free(args[idx]);
				args[idx] = NULL;
				break;
			}
		} else if (args[idx][0] == '|') {
			if (idx == 0) {
				printf("ERROR: Pipe used with no input program specified\n");
			} else if (args[idx+1] == NULL || args[idx+1][0] == '|' || args[idx+1][0] == '>') {
				printf("ERROR: Pipe used with no output program specified\n");
				return;
			} else {
				// if we're here, we know it's safe to pipe the two processes
				pipe_idxs[pipe_idx++] = idx;		

				// Insert new end to args
				free(args[idx]);
				args[idx] = NULL;
			}
		}
	}

	// check for pipes
	int fd_in = 0;
	int exec_idx = 0;
	int next_exec_idx = 0;

	for (int i = 0; pipe_idxs[i] != 0; i++) {
		// create a pipe to link the 2 commands
		int fds[2];
		pipe(fds);

		// fork off a child for the writing process
		child_running = fork();

		if (!child_running) {  // if we are the child
			close(fds[0]);  // close pipe read end
			dup2(fds[1], STDOUT_FILENO);  // attach write end 

			if (fd_in) {
				dup2(fd_in, STDIN_FILENO);  // attach read end		
			}
			
			// only grab the args associated with this executable
			execvp(args[exec_idx], args + exec_idx);
			exit(EXIT_FAILURE);
		} else {  // if we are the parent

			exec_idx = next_exec_idx;
			next_exec_idx = pipe_idxs[i] + 1;

			close(fds[1]);  // close write end
			if (fd_in) {
				close(fd_in);  // close old read end
			}
			fd_in = fds[0];  // save new read end
			pipe_idxs[i] = child_running;  // save the child pid
		}
	}

	// handle final executable
	child_running = fork();

	if (!child_running) {  // child
		// set up pipe if necessary
		if (fd_in) {
			dup2(fd_in, STDIN_FILENO);  // attach read end		
		}

		// set up redirect if necessary
		if (redirect_idx) {
			const char *fname = args[redirect_idx+1];
			// apply bitmask to check for append
			int fd = open(fname, 
				O_WRONLY|O_CREAT|(O_APPEND & (~0*append)),  
				S_IRUSR|S_IWUSR);
			dup2(fd, STDOUT_FILENO);
			args[redirect_idx] = NULL;
		}

		// Reset signal handling and execute the command
		signal(SIGINT, SIG_DFL); 
		execvp(args[next_exec_idx], args + next_exec_idx);
		exit(EXIT_FAILURE);
	} else {  // parent
		if (fd_in) {
			close(fd_in);
		}

		// Wait for all children and handle exit statuses
		// TODO handle exit status
		for (int i = 0; pipe_idxs[i] != 0; i++) {
			waitpid(pipe_idxs[i], &status, 0);
		}
		waitpid(child_running, &status, 0);
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
