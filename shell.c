#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

// --- Configuration Constants ---
#define MAX_LINE 1024       // Maximum input line length
#define MAX_ARGS 64         // Maximum arguments per command
#define MAX_JOBS 10         // Maximum number of background jobs
#define MAX_COMMANDS 10     // Maximum number of commands in a pipeline

// --- Job Management Structure ---
typedef enum { RUNNING, STOPPED, DONE } job_status;

typedef struct job {
    pid_t pid;
    int id;
    char command[MAX_LINE];
    job_status status;
} job_t;

job_t job_list[MAX_JOBS];
int next_job_id = 1;
// Flag to indicate if a SIGCHLD signal arrived
volatile sig_atomic_t children_pending = 0;

// --- Utility and Job Management Functions ---

/**
 * @brief Adds a new job to the job list.
 * @param pid The process ID of the job.
 * @param command The command string.
 * @param is_background 1 if background, 0 otherwise.
 */
void add_job(pid_t pid, const char *command, int is_background) {
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].pid == 0) { // Found empty slot
            job_list[i].pid = pid;
            job_list[i].id = next_job_id++;
            strncpy(job_list[i].command, command, MAX_LINE - 1);
            job_list[i].command[MAX_LINE - 1] = '\0';
            job_list[i].status = is_background ? RUNNING : DONE; // Done for simplicity if not background
            if (is_background) {
                fprintf(stderr, "[%d] %d\n", job_list[i].id, pid);
            }
            return;
        }
    }
    fprintf(stderr, "Error: Job list full.\n");
}

/**
 * @brief Removes a job based on its PID.
 * @param pid The PID of the job to remove.
 */
void remove_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].pid == pid) {
            memset(&job_list[i], 0, sizeof(job_t));
            return;
        }
    }
}

/**
 * @brief Searches for a job by its ID.
 * @param job_id The job ID.
 * @return Pointer to the job_t structure or NULL if not found.
 */
job_t *find_job_by_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].id == job_id && job_list[i].pid > 0) {
            return &job_list[i];
        }
    }
    return NULL;
}

/**
 * @brief Prints the list of running background jobs.
 */
void print_jobs() {
    int count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].pid > 0) {
            const char *status_str = (job_list[i].status == RUNNING) ? "Running" : "Stopped";
            fprintf(stderr, "[%d] %s %s\n", job_list[i].id, status_str, job_list[i].command);
            count++;
        }
    }
    if (count == 0) {
        fprintf(stderr, "No background jobs.\n");
    }
}

// --- Signal Handling ---

/**
 * @brief SIGCHLD handler to reap terminated child processes (prevent zombies).
 */
void sigchld_handler(int sig) {
    children_pending = 1;
    // We use WNOHANG to not block the shell.
    // Loop to clean up multiple children if they exit concurrently.
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Child exited or terminated
            for (int i = 0; i < MAX_JOBS; i++) {
                if (job_list[i].pid == pid) {
                    fprintf(stderr, "\nJob [%d] (%d) Done: %s\n", job_list[i].id, pid, job_list[i].command);
                    remove_job_by_pid(pid);
                    break;
                }
            }
        } else if (WIFSTOPPED(status)) {
            // Child was stopped (e.g., via Ctrl+Z, which we don't fully support here, but it's good practice)
            for (int i = 0; i < MAX_JOBS; i++) {
                if (job_list[i].pid == pid) {
                    job_list[i].status = STOPPED;
                    fprintf(stderr, "\nJob [%d] (%d) Stopped: %s\n", job_list[i].id, pid, job_list[i].command);
                    break;
                }
            }
        }
    }
    children_pending = 0;
}

/**
 * @brief Initializes signal handlers for the shell process.
 */
void init_signals() {
    // Ignore SIGINT (Ctrl+C) in the parent shell process.
    // Foreground children will still receive it and terminate.
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("signal SIGINT");
    }
    // Set up SIGCHLD handler to reap zombies.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}

// --- Parsing and Execution Logic ---

/**
 * @brief Tokenizes a single command string into arguments and handles redirection.
 * @param command_str The raw command string (e.g., "ls -l > output.txt").
 * @param args Array to store tokenized arguments (execvp format).
 * @param input_file Output pointer for input redirection file name (or NULL).
 * @param output_file Output pointer for output redirection file name (or NULL).
 * @param append_flag Output pointer for append redirection (1 if >>, 0 otherwise).
 * @return 1 on success, 0 on parsing failure.
 */
int parse_redirection(char *command_str, char **args, char **input_file, char **output_file, int *append_flag) {
    char *token;
    int i = 0;

    *input_file = NULL;
    *output_file = NULL;
    *append_flag = 0;

    // Use strtok_r for re-entrant tokenizing, safer for complex parsing
    char *saveptr;
    token = strtok_r(command_str, " \t\n", &saveptr);

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " \t\n", &saveptr);
            if (token == NULL) { fprintf(stderr, "Syntax error: Missing input file.\n"); return 0; }
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " \t\n", &saveptr);
            if (token == NULL) { fprintf(stderr, "Syntax error: Missing output file.\n"); return 0; }
            *output_file = token;
            *append_flag = 0; // Overwrite mode
        } else if (strcmp(token, ">>") == 0) {
            token = strtok_r(NULL, " \t\n", &saveptr);
            if (token == NULL) { fprintf(stderr, "Syntax error: Missing output file.\n"); return 0; }
            *output_file = token;
            *append_flag = 1; // Append mode
        } else if (i < MAX_ARGS - 1) {
            args[i++] = token;
        }
        token = strtok_r(NULL, " \t\n", &saveptr);
    }
    args[i] = NULL; // Null-terminate the argument list for execvp()
    return 1;
}

/**
 * @brief Executes a single command, handling redirection.
 * @param command_str The command string.
 * @param fd_in File descriptor for standard input (inherited or pipe read end).
 * @param fd_out File descriptor for standard output (inherited or pipe write end).
 * @param command_full_str The full original command string (for job listing).
 * @param is_background 1 if background job, 0 otherwise.
 */
void execute_single_command(char *command_str, int fd_in, int fd_out, const char *command_full_str, int is_background) {
    char *args[MAX_ARGS];
    char *input_file = NULL, *output_file = NULL;
    int append_flag = 0;

    if (!parse_redirection(command_str, args, &input_file, &output_file, &append_flag)) {
        return; // Parsing error
    }

    if (args[0] == NULL) {
        return; // Empty command
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) { // Child Process
        // Restore default SIGINT handler for the child process
        signal(SIGINT, SIG_DFL);

        // 1. Handle I/O Redirection
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                perror(input_file);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        } else if (fd_in != STDIN_FILENO) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (output_file) {
            int flags = O_WRONLY | O_CREAT;
            if (append_flag) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            int fd = open(output_file, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd < 0) {
                perror(output_file);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (fd_out != STDOUT_FILENO) {
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        // 2. Execute external command
        if (execvp(args[0], args) < 0) {
            fprintf(stderr, "shell: Command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }

    } else { // Parent Process
        // Add job to list (even if foreground, temporarily)
        add_job(pid, command_full_str, is_background);

        // Close pipe ends in parent (important!)
        if (fd_in != STDIN_FILENO) close(fd_in);
        if (fd_out != STDOUT_FILENO) close(fd_out);

        // Wait for foreground job
        if (!is_background) {
            int status;
            // Wait specifically for the foreground child process
            if (waitpid(pid, &status, 0) < 0) {
                // Check if waitpid failed due to an interrupted system call (EINTRA)
                if (errno != EINTR) {
                    perror("waitpid");
                }
            }
            // Clean up the job list after waiting
            remove_job_by_pid(pid);
        }
    }
}

/**
 * @brief Parses the input line for pipelines and background marker.
 * @param line The input line read from stdin.
 * @param command_full_str The full command string (for job listing).
 * @return 1 if handled, 0 if it was an external command/pipeline.
 */
int execute_pipeline_or_builtin(char *line, const char *command_full_str) {
    // 1. Check for background marker '&' and remove it
    int is_background = 0;
    if (line[strlen(line) - 1] == '&') {
        is_background = 1;
        line[strlen(line) - 1] = '\0'; // Null-terminate to remove '&'
        // Also remove any preceding spaces/tabs
        while (strlen(line) > 0 && (line[strlen(line) - 1] == ' ' || line[strlen(line) - 1] == '\t')) {
            line[strlen(line) - 1] = '\0';
        }
    }

    // 2. Tokenize by pipe '|'
    char *commands[MAX_COMMANDS];
    char *pipe_token = strtok(line, "|");
    int num_commands = 0;
    while (pipe_token != NULL && num_commands < MAX_COMMANDS) {
        // Trim leading/trailing whitespace from command parts
        size_t start = strspn(pipe_token, " \t");
        size_t end = strlen(pipe_token) - 1;
        while(end > start && (pipe_token[end] == ' ' || pipe_token[end] == '\t' || pipe_token[end] == '\n')) end--;
        pipe_token[end + 1] = '\0';

        commands[num_commands++] = pipe_token + start;
        pipe_token = strtok(NULL, "|");
    }

    if (num_commands == 0) return 0; // Empty line

    // 3. Built-in command check (only for the first command in a pipeline)
    char *arg0_copy = strdup(commands[0]); // Need a copy for parsing the command name
    char *arg0_name = strtok(arg0_copy, " \t\n");

    int handled = 0;

    if (arg0_name) {
        if (strcmp(arg0_name, "exit") == 0) {
            // Reap any remaining background jobs before exiting
            for (int i = 0; i < MAX_JOBS; i++) {
                if (job_list[i].pid > 0) {
                    kill(job_list[i].pid, SIGTERM);
                }
            }
            fprintf(stderr, "Exiting mini-shell.\n");
            free(arg0_copy);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(arg0_name, "cd") == 0) {
            // cd built-in (command 0 only, no redirection/pipes)
            char *path = strtok(NULL, " \t\n");
            if (path == NULL) {
                path = getenv("HOME");
            }
            if (chdir(path) != 0) {
                perror("cd");
            }
            handled = 1;
        }
        else if (strcmp(arg0_name, "jobs") == 0) {
            // jobs built-in
            print_jobs();
            handled = 1;
        }
        else if (strcmp(arg0_name, "fg") == 0) {
            // fg built-in
            char *job_id_str = strtok(NULL, " \t\n");
            if (job_id_str) {
                int job_id = atoi(job_id_str);
                job_t *job = find_job_by_id(job_id);
                if (job) {
                    fprintf(stderr, "Bringing job [%d] (%d) to foreground: %s\n", job->id, job->pid, job->command);
                    // Send SIGCONT to the process to resume it if it was stopped
                    if (kill(job->pid, SIGCONT) < 0) {
                         if (errno != ESRCH) perror("kill SIGCONT");
                    }

                    // Wait for the job to finish
                    int status;
                    if (waitpid(job->pid, &status, 0) < 0) {
                        if (errno != EINTR) perror("waitpid fg");
                    }
                    remove_job_by_pid(job->pid);
                } else {
                    fprintf(stderr, "shell: fg: No such job: %s\n", job_id_str);
                }
            } else {
                fprintf(stderr, "shell: fg: usage: fg <job_id>\n");
            }
            handled = 1;
        }
    }

    free(arg0_copy);

    if (handled) {
        return 1;
    }

    // 4. Handle External Command/Pipeline
    int i;
    int fd_in = STDIN_FILENO;
    int pipefd[2];

    for (i = 0; i < num_commands; i++) {
        int fd_out = STDOUT_FILENO;

        // If this is not the last command, set up the pipe
        if (i < num_commands - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return 0;
            }
            fd_out = pipefd[1]; // Pipe's write end is the current command's output
        }

        // Execute the command part
        execute_single_command(commands[i], fd_in, fd_out, command_full_str, is_background && (i == num_commands - 1));

        // If not the last command, close the write end in the parent
        // and set the read end as the input for the next command
        if (i < num_commands - 1) {
            close(pipefd[1]);
            fd_in = pipefd[0]; // Pipe's read end is the next command's input
        }
    }

    return 1;
}

/**
 * @brief The main loop of the shell.
 */
void shell_loop() {
    char line[MAX_LINE];
    char *command_full_str;

    while (1) {
        // Print the prompt
        fprintf(stderr, "minish> ");

        // Read input line
        if (fgets(line, MAX_LINE, stdin) == NULL) {
            // Handle EOF (Ctrl+D)
            fprintf(stderr, "\n");
            break;
        }

        // Remove trailing newline
        if (line[0] != '\0' && line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        // Ignore empty lines
        if (strlen(line) == 0) continue;

        // Create a copy of the line before tokenization (needed for job list printing)
        command_full_str = strdup(line);
        if (command_full_str == NULL) {
            perror("strdup");
            continue;
        }

        execute_pipeline_or_builtin(line, command_full_str);

        free(command_full_str);
    }
}

/**
 * @brief Main function.
 */
int main() {
    // Clear the job list at startup
    memset(job_list, 0, sizeof(job_list));

    // Initialize signal handlers
    init_signals();

    // Start the shell loop
    shell_loop();

    return 0;
}
