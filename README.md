![Mini Unix Shell](https://raw.githubusercontent.com/Cr4xen/Mini-Unix-Shell/main/unix.jpg)

# MiniShell (minish)

[![Language: C](https://img.shields.io/badge/Language-C-blue?style=flat-square)](https://www.gnu.org/software/gnu-c-manual/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green?style=flat-square)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen?style=flat-square)]()

`minish` is a **POSIX-compliant UNIX-like shell** implemented in C. It provides a minimal yet robust environment for command execution, I/O redirection, piping, background processing, and job control. Ideal for system programming enthusiasts and students learning shell fundamentals.

---

## Key Features

### Command Execution

* Execute external commands using `fork()` and `execvp()`.

### Built-in Commands

* `exit` – Terminates the shell.
* `cd [path]` – Changes the current working directory.
* `jobs` – Lists background jobs.
* `fg <job_id>` – Brings a background job to the foreground.

### I/O Redirection

* `< input_file` – Redirect input from a file.
* `> output_file` – Redirect output and overwrite the file.
* `>> output_file` – Redirect output and append to the file.

### Piping & Background Execution

* `|` – Connect commands via pipes.
* `&` – Run commands in the background.

### Signal Handling

* `SIGCHLD` – Properly handles background process termination.
* `SIGINT` (Ctrl+C) – Forwarded to foreground processes while ignoring the shell itself.

---

## Installation & Usage

### Compile

```bash
make
```

Creates the executable `minish`.

### Run

```bash
./minish
```

### Example Commands

| Feature              | Command                     | Description                   |                           |
| -------------------- | --------------------------- | ----------------------------- | ------------------------- |
| Simple Command       | `ls -l`                     | List directory contents.      |                           |
| Change Directory     | `cd ..`                     | Move to parent directory.     |                           |
| Redirect Output      | `echo "Hello" > file.txt`   | Write to a file.              |                           |
| Append Output        | `echo " World" >> file.txt` | Append to a file.             |                           |
| Pipe Commands        | `ls -l                      | grep file`                    | Filter output using grep. |
| Background Execution | `sleep 5 &`                 | Run process in background.    |                           |
| Job Listing          | `jobs`                      | Display background jobs.      |                           |
| Foreground Job       | `fg 1`                      | Bring job ID 1 to foreground. |                           |

---

## Why Choose Minish?

* **Educational:** Understand process management, I/O handling, and signal processing.
* **Lightweight:** Minimal dependencies, runs on any POSIX-compliant system.
* **Extendable:** Easy to add new features or built-in commands for experimentation.

---

## License

This project is licensed under the [MIT License](LICENSE).
