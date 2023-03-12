# Simple Shell (Multi-Processing)

This is a basic shell program for Linux that supports basic commands such as `exit`, `pwd`, `clear`, `cd`, `echo`, `rm`, and `mkdir`. It also includes environment management with `setenv`, `unsetenv`, and `getenv`, and program invocation with forking and child processes. Additionally, it supports background execution of programs with `&` at the end of the command, and sends a `SIGINT` signal when `Ctrl-C` is pressed (the shell is not exited).

## How to run

To compile and run the program, follow these steps:

1. Open a terminal window and navigate to the directory containing the `main.c` file.
2. Compile the program using the following command:

<font color="green">`gcc -o shell main.c`</font>

3. Run the program by entering the following command:

<font color="green">./shell</font>



## Usage

The shell supports the following commands:

- `exit`: Exit the shell.
- `pwd`: Print the current working directory.
- `clear`: Clear the terminal window.
- `cd`: Change the current working directory. Usage: `cd [directory]`.
- `echo`: Print the input to the terminal. Usage: `echo [message]`.
- `rm`: Remove a file or directory. Usage: `rm [file/directory]`.
- `mkdir`: Create a new directory. Usage: `mkdir [directory]`.
- `setenv`: Set an environment variable. Usage: `setenv [variable] [value]`.
- `unsetenv`: Unset an environment variable. Usage: `unsetenv [variable]`.
- `getenv`: Get the value of an environment variable. Usage: `getenv [variable]`.



