# KeldShell

KeldShell is a Unix shell written in C.

It reads commands from the user, parses them, builds an Abstract Syntax Tree (AST), and executes the commands with Unix system calls.

The project includes command execution, pipes, redirections, logical operators, subshells, background processes, job control, signal handling, variable expansion, wildcard expansion, here-documents, and command history.

The main goal of this project was to understand how a Unix shell works internally.


## Features

* External commands with fork() and execvp()

* Built-in commands

* Command pipelines with |

* Logical operators with && and ||

* Sequential commands with ;

* Background execution with &

* Subshells with ( )

* Input and output redirections

* Standard error redirections

* Here-documents

* Environment variable expansion with `$VAR` and `${VAR}`

* Special variable expansion such as `$?` and `$$`

* Single and double quotes

* Escape characters

* Wildcards with * and ?

* Command history

* Process groups

* Foreground and background jobs

* jobs, fg, and bg

* Ctrl-C and Ctrl-Z signal handling

* Interactive input with GNU Readline

## Build
### Requirements
KeldShell is designed for Unix systems.

You need:

* GCC
* GNU Make
* GNU Readline
* A POSIX-compatible environment

GNU Readline can be installed with:
```
sudo apt install libreadline-dev
```

### Compile
This creates the executable:
```
make
```

Run it with:
```
./keld
```

To remove the compiled executable:
```
make clean
```

To rebuild the project:
```
make re
```


