# Custom C Shell — Implementation Overview

This project implements a fully functional Unix-like shell in C, adhering strictly to POSIX standards. The shell is developed incrementally, supporting command parsing, execution, redirection, pipes, job control, and custom shell intrinsics.

The implementation is modular, split across multiple `.c` and `.h` files based on functionality, and compiled with strict POSIX-compliant GCC flags.

---

## General Design & Compliance

- Code is organized across multiple source and header files based on functionality.
- Only POSIX C libraries and system calls are used.
- Compilation uses strict flags to enforce correctness and standards compliance.
- The shell builds successfully using `make all`, producing `shell.out` in the `shell/` directory.
- All invalid inputs and edge cases are handled with explicit error messages.

---

## Shell Input & Parsing

### Shell Prompt

- Implemented a dynamic shell prompt in the format:
<Username@SystemName:current_path>


- The shell:
- Detects the username and system name dynamically.
- Treats the directory where the shell starts as the home directory.
- Displays `~` when the current directory is within the home directory.
- Displays absolute paths otherwise.
- The prompt is shown only when no foreground process is running.

---

### User Input Handling

- The shell:
- Accepts arbitrary user input.
- Consumes input on pressing Enter.
- Re-displays the prompt after each input.
- At this stage, input is read and parsed even if no execution occurs.

---

### Command Parsing

- Implemented a full parser based on the provided Context Free Grammar (CFG).
- The parser:
- Correctly handles command groups, pipelines, redirections, background (`&`) and sequential (`;`) operators.
- Ignores arbitrary whitespace between tokens.
- Validates syntax before execution.
- Invalid commands result in:
- Parsing is independent of execution logic and does not rely on an AST.

---

## Shell Intrinsics

### `hop` — Directory Navigation

- Implements directory traversal similar to `cd`.
- Supports:
- `~` or no argument → home directory
- `.` → current directory
- `..` → parent directory
- `-` → previous working directory
- Relative and absolute paths
- Tracks previous working directory internally.
- Outputs `No such directory!` on failure.

---

### `reveal` — Directory Listing

- Implements directory listing similar to `ls`.
- Supports flags:
- `-a` → show hidden files
- `-l` → line-by-line output
- Behaves identically to `hop` for path resolution.
- Lists files in lexicographic (ASCII) order.
- Handles invalid syntax, excessive arguments, and missing directories with appropriate error messages.

---

### `log` — Command History

- Maintains a persistent command history across shell sessions.
- Stores up to 15 commands, overwriting the oldest.
- Does not store:
- Consecutive duplicate commands
- Commands where the atomic command is `log`
- Supports:
- `log` → display history
- `log purge` → clear history
- `log execute <index>` → execute command by index without re-logging
- Handles invalid syntax explicitly.

---

##  Execution, Redirection & Pipes

### Command Execution

- Supports execution of arbitrary external commands (`cat`, `echo`, `sleep`, etc.).
- Uses `fork`, `exec`, and `wait` correctly.
- Prints `Command not found!` for invalid commands.

---

### Input Redirection (`<`)

- Redirects standard input using `open()` and `dup2()`.
- Supports multiple input redirections, with the last one taking effect.
- Gracefully handles missing or unreadable files.

---

### Output Redirection (`>` and `>>`)

- Supports overwrite (`>`) and append (`>>`) modes.
- Creates files when needed.
- Handles file creation failures with appropriate errors.
- Works correctly in combination with input redirection.

---

### Command Piping (`|`)

- Implements multi-stage pipelines using `pipe()`.
- Each command runs in its own child process.
- Correctly connects standard input/output across the pipeline.
- Waits for all pipeline processes to terminate.
- Continues executing remaining pipeline commands even if one fails.
- Supports mixing pipes with file redirection.

---

## Sequential & Background Execution

### Sequential Execution (`;`)

- Executes commands strictly in order.
- Waits for each command to finish before starting the next.
- Continues execution even if a command fails.
- Displays the prompt only after all commands finish.

---

### Background Execution (`&`)

- Executes commands asynchronously.
- Prints job number and PID upon launch.
- Immediately returns control to the shell.
- Tracks background processes and reports their completion status.
- Prevents background processes from reading terminal input.

---

##  Advanced Shell Features

### `activities` — Process Tracking

- Lists all active or stopped processes spawned by the shell.
- Displays: [pid]: command_name - State
- Sorts output lexicographically by command name.
- Automatically removes terminated processes from tracking.

---

### `ping` — Signal Handling

- Sends signals to processes using `kill()`.
- Applies modulo 32 to signal numbers.
- Handles invalid PIDs and invalid signal numbers gracefully.

---

### Job Control: Ctrl-C, Ctrl-D, Ctrl-Z

- **Ctrl-C (SIGINT):**
- Sends SIGINT to foreground process group.
- Does not terminate the shell.

- **Ctrl-D (EOF):**
- Terminates all child processes.
- Prints `logout` and exits cleanly.

- **Ctrl-Z (SIGTSTP):**
- Stops foreground process.
- Moves it to background job list.
- Displays job number and status.

---

## Summary

This project delivers a fully functional POSIX-compliant shell with:

- Robust parsing based on formal grammar
- Complete I/O redirection and piping
- Background and sequential execution
- Persistent command history
- Process tracking and job control
- Modular, maintainable C code

The shell closely mirrors the behavior of standard Unix shells while adhering strictly to assignment constraints and POSIX standards.

## Instructions to Run:
```bash
make all
./shell.out
```

Further details and descriptions can be found in: 
https://karthikv1392.github.io/cs3301_osn/mini-projects/mini-project1
