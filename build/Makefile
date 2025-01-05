# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pthread -lncurses

# Directories
SRC_DIR = ../src
BLD_DIR = .

# Targets
TARGETS = server client

# Default target
all: $(TARGETS)

# Server target
server:	$(SRC_DIR)/server.c
	$(CC) -o $(BLD_DIR)/server $(SRC_DIR)/server.c $(CFLAGS)

# Client target
client: $(SRC_DIR)/client.c
	$(CC) -o $(BLD_DIR)/client $(SRC_DIR)/client.c $(CFLAGS)

run-client: client
	$(BLD_DIR)/client
