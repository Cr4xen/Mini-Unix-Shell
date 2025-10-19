# MiniShell (minish)

`minish` is a simple, POSIX-compliant UNIX-like shell written in C. It supports external commands, command arguments, built-in commands, I/O redirection, piping, and basic job control.

## Features

1. **Command Execution:** Runs external commands using `fork()` and `execvp()`.
    
2. **Built-ins:**
    
    - `exit`: Terminates the shell.
        
    - `cd [path]`: Changes the current working directory.
        
    - `jobs`: Lists background processes.
        
    - `fg <job_id>`: Brings a background job to the foreground and waits for it to complete.
        
3. **I/O Redirection:**
    
    - `< input_file`: Redirects standard input.
        
    - `> output_file`: Redirects standard output (overwrites file).
        
    - `>> output_file`: Redirects standard output (appends to file).
        
4. **Piping:** Connects the standard output of one command to the standard input of another (`|`).
    
5. **Background Execution:** Runs a command in the background using the `&` suffix.
    
6. **Signal Handling:**
    
    - `SIGCHLD`: Handled correctly to reap zombie processes from background jobs.
        
    - `SIGINT` (Ctrl+C): Ignored by the shell process, but passed to foreground jobs to terminate them.
        

## Usage

### 1. Compilation

Use the provided `Makefile` to compile the shell:

```
make
```

This will create an executable file named `minish`.

### 2. Running the Shell

```
./minish
```

### 3. Examples

|Feature|Command|Description|
|---|---|---|
|**Simple Command**|`ls -l`|Lists files in the current directory.|
|**Built-in**|`cd ..`|Moves to the parent directory.|
|**Redirection (Overwrite)**|`echo "Hello" > file.txt`|Writes "Hello" to `file.txt`.|
|**Redirection (Append)**|`echo " World" >> file.txt`|Appends " World" to `file.txt`.|
|**Piping**|`ls -l|grep file`|
|**Background Job**|`sleep 5 &`|Runs the `sleep` command in the background.|
|**Job Control**|`jobs`|Lists the running background jobs (e.g., `sleep 5`).|
|**Job Control**|`fg 1`|Brings job ID 1 (e.g., the `sleep 5` job) back to the foreground.|
