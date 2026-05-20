CC = cc
CFLAGS = -Wall -Wextra
SRC = main.c tui.c
OUT = nocurse

.PHONY: all run clean re

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

re: clean all run

run: all
	./$(OUT)

clean:
	rm -rf $(OUT)
