CC = gcc
CFLAGS = -Wall -Werror
SRC = shell.c helpers.c builtins.c
OBJ = $(SRC:.c=.o)
EXEC = shell

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
