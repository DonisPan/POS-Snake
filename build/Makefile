# compiler
CC = gcc

CFLAGS = -Wall -Wextra
LFLAGS = -lncurses -pthread

SRC_DIR = ../src
BLD_DIR = .

# targets
TARGETS = server client

# files
SERVER_SRCS = $(SRC_DIR)/server.c $(SRC_DIR)/map.c $(SRC_DIR)/snake.c
CLIENT_SRCS = $(SRC_DIR)/client.c

# all
all: $(TARGETS)

# server
server:	$(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $(BLD_DIR)/server $(SERVER_SRCS) $(LFLAGS)

# client
client: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $(BLD_DIR)/client $(CLIENT_SRCS) $(LFLAGS)

run-client: client
	$(BLD_DIR)/client

clean:
	rm -f $(BLD_DIR)/server $(BLD_DIR)/client
