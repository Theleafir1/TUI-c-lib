CC = g++
CFLAGS = -Wall -Wextra
SRC = main.cpp tui.cpp
OUT = nocurse

.PHONY: all run clean re

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

re: clean all run

run: all
	./$(OUT)

clean:
	rm -rf $(OUT)
