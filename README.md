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

## Usage
After starting KeldShell, the prompt shows the current working directory.
```
keld>> /home/user/projects >>
```
### External Commands
A normal external command can be executed directly.
#### Example:
```
ls -la
```
```
cat file.txt
```
### Built-in Commands
KeldShell contains several built-in commands.
| Commands | Description |
| :--- | :--- |
| cd | Changes the current working directory. |
| pwd | Prints the current working directory. |
| echo | Prints arguments. |
| export | Creates or updates an environment variable. |
| unset | Removes an environment variable. |
| history | Prints command history. |
| jobs | Lists active and stopped jobs. |
| fg | Moves a job to the foreground. |
| bg | Continues a stopped job in the background. |
| clear | Clears the terminal. |
| help | Prints the help menu. |
| exit | Exits KeldShell. |

#### Example:
```
echo hello world
```
```
pwd
```

## Operators
### Pipe
The pipe operator connects the standard output of one command to the standard input of another command.
```
cat file.txt | grep hello | wc -l
```
Each command in the pipeline runs in its own process.
### AND Operator
&& executes the right command only if the left command succeeds.
```
mkdir test && cd test
```
### OR Operator
|| executes the right command only if the left command fails.
```
cat missing.txt || echo "File was not found"
```
### Semicolon
; executes commands in sequence.
```
echo first ; echo second
```
### Background Operator
& runs a command without waiting for it to finish.
```
sleep 30 &
```
Background jobs are stored in the shell job list.

## Redirections
### Standart Input
The file becomes the standard input of the command.
```
cat < input.txt
```
### Standart Output
Creates or truncates the file.
```
echo hello > output.txt
```
### Standard Output Append
Writes to the end of the file.
```
echo hello >> output.txt
```
### Standard Error
Redirects standard error.
```
ls missing_file 2> errors.txt
```
### Standard Error Append
Appends standard error to a file.
```
ls missing_file 2>> errors.txt
```
### Here-Document
#### Example:
```
cat << EOF
hello
world
EOF
```
The shell reads input until the delimiter is entered. Variable expansion is supported inside an unquoted here-document delimiter.
A quoted delimiter keeps the content literal.
#### Example:
```
cat << 'EOF'
hello
world
EOF
```
## Variable Expansion
Environment variables can be used inside commands.
Variables can be written in two forms:
```
echo $HOME
echo ${HOME}/file
```
Output:
```
/home/user
/home/user/file
```
KeldShell also supports special values.
### Last Exit Status
$? contains the exit status of the previous command.
```
echo $?
```
| Status | Code |
| :--- | :--- |
| Success | 0 |
| General Errors | 1 |
| Command Not Found| 127 |
| Terminated by User (Ctrl+C)| 130 |
### Shell Process ID
$$ expands to the process ID of KeldShell.
```
echo $$
```
## Quotes and Escaping
### Single Quote
Single quotes preserve the text literally.
#### Input:
```
echo '$HOME'
```
#### Output:
```
$HOME
```
### Double Quote
Double quotes preserve spaces but still allow variable expansion.
#### Input:
```
echo "Path: $HOME"
```
#### Output:
```
Path: /home/user 
```
### Escaping
Special characters can be escaped with \.
## Wildcard Expansion
It supports:
```
*
?
```
* matches zero or more characters.
#### Example:
```
echo *.c
```
? matches one character.
#### Example:
```
echo file?.txt
```
KeldShell reads entries from the current directory and matches them against the pattern.

## How KeldShell Works
A command passes through several stages before it is executed.

```mermaid
graph TD
    A([Başlat]) --> B[Sonsuz Döngü / Prompt Göster]
    B --> C[Kullanıcı Girişi Al]
    C --> D{Giriş Boş mu?}
    D -- Evet --> B
    D -- Hayır --> E[Komut Ayrıştırma / Parsing]
    E --> F{Yerleşik Komut mu?<br>cd, exit, help vb.}
    F -- Evet --> G[Yerleşik Komutu Çalıştır]
    F -- Hayır --> H[Fork ve exec ile Çalıştır]
    G --> B
    H --> B
    G -. exit .- -> I([Çıkış])
```
