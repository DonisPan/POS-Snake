#include <arpa/inet.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT 1234

#define MAP_WIDTH 25
#define MAP_HEIGHT 25
#define MAX_SNACKS 50
#define MAX_PLAYERS 10
#define SNAKE_MAX_LENGTH 256
#define SNAKE_SPEED 500000
bool game_end = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int x;
  int y;
} Snake_bit;

typedef struct {
  Snake_bit parts[SNAKE_MAX_LENGTH];
  int length;
  int dirX;
  int dirY;
  bool playing;
} Snake;

typedef struct {
  int x;
  int y;
  bool chomped;
} Snack;

typedef struct {
  int id;
  int client_socket;
  Snake *snake;
} Client_data;

Snack snacks[MAX_SNACKS];
Snake snakes[MAX_PLAYERS];
char map[MAP_WIDTH * MAP_HEIGHT];
int client_sockets[MAX_PLAYERS];

void spawn_snake(Snake *snake);
void spawn_snacks();
void generate_map();
void move_snakes();
void broadcast_map();
void *handle_client(void *args);
void *game_loop(void *args);

void spawn_snake(Snake *snake) {
  snake->length = 1;
  snake->parts[0].x = 1 + rand() % (MAP_WIDTH - 2);
  snake->parts[0].y = 1 + rand() % (MAP_HEIGHT - 2);
  snake->dirX = 1;
  snake->dirY = 0;
  snake->playing = true;
}

void spawn_snacks() {
  for (int i = 0; i < MAX_SNACKS; ++i) {
    snacks[i].x = 1 + rand() % (MAP_WIDTH - 2);
    snacks[i].y = 1 + rand() % (MAP_HEIGHT - 2);
    snacks[i].chomped = false;
  }
}

void generate_map() {
  // map border
  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      if (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) {
        map[y * MAP_WIDTH + x] = '#';
      } else {
        map[y * MAP_WIDTH + x] = ' ';
      }
    }
  }

  // snacks
  for (int i = 0; i < MAX_SNACKS; ++i) {
    if (!snacks[i].chomped) {
      map[snacks[i].y * MAP_WIDTH + snacks[i].x] = '*';
    }
  }

  // snakes
  for (int s = 0; s < MAX_PLAYERS; ++s) {
    Snake *snake = &snakes[s];
    if (!snake->playing) {
      continue;
    }
    for (int i = 0; i < snake->length; ++i) {
      map[snake->parts[i].y * MAP_WIDTH + snake->parts[i].x] =
          (i == 0) ? '0' : 'o';
    }
  }
}

void move_snakes() {
  for (int s = 0; s < MAX_PLAYERS; ++s) {
    // move snake
    Snake *snake = &snakes[s];
    if (!snake->playing) {
      continue;
    }

    for (int i = snake->length - 1; i > 0; --i) {
      snake->parts[i] = snake->parts[i - 1];
    }
    snake->parts[0].x += snake->dirX;
    snake->parts[0].y += snake->dirY;

    // wall collisions
    if (snake->parts[0].x <= 0 || snake->parts[0].x >= MAP_WIDTH - 1 ||
        snake->parts[0].y <= 0 || snake->parts[0].y >= MAP_HEIGHT - 1) {
      snake->playing = false;
    }

    // self collisions
    for (int i = 1; i < snake->length; ++i) {
      if (snake->parts[0].x == snake->parts[i].x &&
          snake->parts[0].y == snake->parts[i].y) {
        snake->playing = false;
      }
    }

    // snack collisions
    for (int i = 0; i < MAX_SNACKS; ++i) {
      if (!snacks[i].chomped && snacks[i].x == snake->parts[0].x &&
          snacks[i].y == snake->parts[0].y) {
        snacks[i].chomped = true;
        snake->length++;
      }
    }
  }
}

void broadcast_map() {
  pthread_mutex_lock(&mutex);
  generate_map();
  for (int s = 0; s < MAX_PLAYERS; ++s) {
    if (!snakes[s].playing) {
      continue;
    }
    send(client_sockets[s], map, sizeof(map), 0);
    pthread_mutex_unlock(&mutex);
  }
}

void *game_loop(void *args) {
  while (!game_end) {
    pthread_mutex_lock(&mutex);
    move_snakes();
    generate_map();
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < MAX_PLAYERS; ++i) {
      if (!snakes[i].playing) {
        continue;
      }
      send(client_sockets[i], map, sizeof(map), 0);
    }

    // broadcast_map();
    usleep(SNAKE_SPEED);
  }
  return NULL;
}

void *handle_client(void *args) {
  Client_data *client_data = (Client_data *)args;
  Snake *snake = client_data->snake;

  printw("Client %d connected.\n", client_data->id);
  client_sockets[client_data->id] = client_data->client_socket;

  while (snake->playing) {
    char buffer[1];
    recv(client_data->client_socket, buffer, 1, 0);

    pthread_mutex_lock(&mutex);
    switch (buffer[0]) {
    case 'w':
      snake->dirX = 0;
      snake->dirY = -1;
      break;
    case 's':
      snake->dirX = 0;
      snake->dirY = 1;
      break;
    case 'a':
      snake->dirX = -1;
      snake->dirY = 0;
      break;
    case 'd':
      snake->dirX = 1;
      snake->dirY = 0;
      break;
    case 'x':
      game_end = true;
    default:
      break;
    }
    pthread_mutex_unlock(&mutex);
  }

  close(client_data->client_socket);
  snake->playing = false;
  printf("Client %d disconnected.\n", client_data->id);
  free(client_data);

  return NULL;
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  spawn_snacks();

  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Socket creation failed!\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr *)&server_address,
           sizeof(server_address)) == -1) {
    perror("Bind failed!\n");
    return EXIT_FAILURE;
  }

  if (listen(server_socket, MAX_PLAYERS) == -1) {
    perror("Listen failed!\n");
    return EXIT_FAILURE;
  }

  printf("Server is listening on port: %d\n", PORT);

  pthread_t game_thread;
  pthread_create(&game_thread, NULL, game_loop, NULL);

  int client_id = 0;
  while (!game_end) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(client_address);

    int client_socket = accept(
        server_socket, (struct sockaddr *)&client_address, &client_length);
    if (client_socket == -1) {
      perror("Accept failure!\n");
      continue;
    }

    Client_data *client_data = malloc(sizeof(Client_data));
    client_data->id = client_id;
    ++client_id;
    client_data->client_socket = client_socket;
    client_data->snake = &snakes[client_data->id];
    spawn_snake(client_data->snake);

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_client, client_data);
    pthread_detach(client_thread);
  }

  close(server_socket);
  pthread_mutex_destroy(&mutex);

  return EXIT_SUCCESS;
}
