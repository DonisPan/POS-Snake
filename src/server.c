#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT 1234
#define BUFFER_SIZE 1024

#define MAP_WIDTH 25
#define MAP_HEIGHT 25
#define MAX_SNACKS 50
#define SNAKE_MAX_LENGTH 256

typedef struct {
  int x;
  int y;
} Snake_bit;

typedef struct {
  int length;
  Snake_bit parts[SNAKE_MAX_LENGTH];
} Snake;

typedef struct {
  int x;
  int y;
  bool chomped;
} Snack;

Snake snake;
Snack snacks[MAX_SNACKS];

char map[MAP_WIDTH * MAP_HEIGHT];

bool game_end = false;

void spawn_snake();
void spawn_snacks();
void move_snake(int deltaX, int deltaY);
void game_rules();
void generate_map();
void handle_client(int client_socket);

void spawn_snake() {
  snake.length = 1;
  snake.parts[0].x = 1 + rand() % (MAP_WIDTH - 2);
  snake.parts[0].y = 1 + rand() % (MAP_HEIGHT - 2);
}

void spawn_snacks() {
  for (int i = 0; i < MAX_SNACKS; ++i) {
    snacks[i].x = 1 + rand() % (MAP_WIDTH - 2);
    snacks[i].y = 1 + rand() % (MAP_HEIGHT - 2);
    snacks[i].chomped = false;
  }
}

void move_snake(int deltaX, int deltaY) {
  for (int i = snake.length - 1; i > 0; --i) {
    snake.parts[i] = snake.parts[i - 1];
  }
  snake.parts[0].x += deltaX;
  snake.parts[0].y += deltaY;
}

void game_rules() {
  for (int i = 0; i < MAX_SNACKS; ++i) {
    if (!snacks[i].chomped && snacks[i].x == snake.parts[0].x &&
        snacks[i].y == snake.parts[0].y) {
      snacks[i].chomped = true;
      snake.length++;
    }
  }

  if (snake.parts[0].x == 0 || snake.parts[0].x == MAP_WIDTH - 1 ||
      snake.parts[0].y == 0 || snake.parts[0].y == MAP_HEIGHT - 1) {
    game_end = true;
  }

  for (int i = 1; i < snake.length; ++i) {
    if (snake.parts[0].x == snake.parts[i].x &&
        snake.parts[0].y == snake.parts[i].y) {
      game_end = true;
    }
  }
}

void generate_map() {
  memset(map, ' ', sizeof(map));

  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      if (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) {
        map[y * MAP_WIDTH + x] = '#';
      }
    }
  }

  for (int i = 0; i < MAX_SNACKS; ++i) {
    if (!snacks[i].chomped) {
      map[snacks[i].y * MAP_WIDTH + snacks[i].x] = '*';
    }
  }

  for (int i = 0; i < snake.length; ++i) {
    map[snake.parts[i].y * MAP_WIDTH + snake.parts[i].x] = (i == 0) ? '0' : 'o';
  }
}

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE];
  while (!game_end) {
    memset(buffer, 0, BUFFER_SIZE);

    recv(client_socket, buffer, BUFFER_SIZE, 0);

    switch (buffer[0]) {
    case 'w':
      move_snake(0, -1);
      break;
    case 's':
      move_snake(0, 1);
      break;
    case 'a':
      move_snake(-1, 0);
      break;
    case 'd':
      move_snake(1, 0);
      break;
    default:
      break;
    }

    game_rules();

    generate_map();

    // snprintf(buffer, BUFFER_SIZE, "Snake score: %d", snake.length);
    send(client_socket, map, strlen(map), 0);
  }

  close(client_socket);
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  spawn_snake();
  spawn_snacks();

  int server_socket;
  int client_socket;

  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t client_len = sizeof(client_address);

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Socket creation failed!\n");
    return EXIT_FAILURE;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr *)&server_address,
           sizeof(server_address)) == -1) {
    perror("Bind failed!\n");
    return EXIT_FAILURE;
  }

  if (listen(server_socket, 1) == -1) {
    perror("Listen failed!\n");
    return EXIT_FAILURE;
  }

  printf("Server is listening on port: %d\n", PORT);

  client_socket =
      accept(server_socket, (struct sockaddr *)&client_address, &client_len);
  if (client_socket == -1) {
    perror("Accept failed!");
    return EXIT_FAILURE;
  }

  printf("Client connected\n");

  handle_client(client_socket);

  close(server_socket);

  return EXIT_SUCCESS;
}
