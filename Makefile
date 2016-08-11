NAME    := killusb
FLAGS   := -lusb -Wall -Wextra
BIN_DIR := /usr/local/bin

all: $(NAME).c $(NAME).h
	gcc -o $(NAME) $< $(FLAGS)

clean:
	rm -f $(NAME)

install:
	install -m 0755 $(NAME) $(BIN_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(NAME)