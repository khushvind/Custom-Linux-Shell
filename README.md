# Custom Linux Shell: Command Execution, Piping, and Built-In Functionalities

## Overview
This project implements a basic Linux shell similar to the bash shell, providing functionality for executing user commands, handling pipes, and implementing built-in commands like `cd`, `history`, and `exit`. The shell interacts with the user through a prompt (MTL458 >) and can handle common Linux commands, user interrupts, and basic error handling.

## Features
### Command Execution:

Supports execution of standard Linux commands like `ls`, `cat`, `echo`, `grep`, `sleep`, etc.
Executes commands using the exec family of system calls.
Continuously processes commands until interrupted by the user or an exit command.

### Piping Support:
Handles single pipe (|) between two commands, allowing the output of one command to be used as input to another.
Example: `grep -o foo file | wc -l`.

### Built-in Commands:
`cd <directory>`: Changes the current working directory of the shell.
`history`: Displays the history of commands entered by the user.
`exit`: Terminates the shell program.

### Error Handling:
Gracefully handles invalid commands or arguments by displaying the message Invalid Command without crashing.
Displays appropriate error messages if any command fails to execute.

### User Interrupt:
The shell terminates cleanly when the `exit` command is issued.