CC = gcc
CFLAGS = -Wall -Wextra -Werror -g3 -fno-omit-frame-pointer

NAME = keld

SRC = main.c \
      builtin.c \
      history.c \
      parser.c \
      redirection.c \
      terminal.c \
      signals.c \
      lexer.c \
      expansion.c \
      prepare.c \
      heredoc.c \
      heredoc_expand.c \
      fd_utils.c \
      execute.c \
      cleanup.c \
      conversion.c \
      utils.c \
      wildcard.c \
      ast.c \
      job_control.c     

all: $(NAME)

$(NAME): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lreadline -o $(NAME)

clean:
	rm -f $(NAME)

re: clean all

.PHONY: all clean re