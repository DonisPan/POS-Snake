#include <arpa/inet.h>
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
#define SNAKE_MAX_LENGTH 256
#define SNAKE_SPEED 500000

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
} Snake;

typedef struct {
  int x;
  int y;
  bool chomped;
} Snack;

typedef struct {
  int id;
  int client_socket;
  Snake snake;
} Client_data;

Snack snacks[MAX_SNACKS];
char map[MAP_WIDTH * MAP_HEIGHT];

void spawn_snake(Snake *snake);
void spawn_snacks();
void generate_map(Snake *snake);
void *handle_client(void *args);
void *move_snake(void *args);

void spawn_snake(Snake *snake) {
  snake->length = 1;
  snake->parts[0].x = 1 + rand() % (MAP_WIDTH - 2);
  snake->parts[0].y = 1 + rand() % (MAP_HEIGHT - 2);
  // snake.parts[0].x = 1;
  // snake.parts[0].y = 1;
  snake->dirX = 1;
  snake->dirY = 0;
}

void spawn_snacks() {
  for (int i = 0; i < MAX_SNACKS; ++i) {
    snacks[i].x = 1 + rand() % (MAP_WIDTH - 2);
    snacks[i].y = 1 + rand() % (MAP_HEIGHT - 2);
    snacks[i].chomped = false;
  }
}

void generate_map(Snake *snake) {
  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      if (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) {
        map[y * MAP_WIDTH + x] = '#';
      } else {
        map[y * MAP_WIDTH + x] = ' ';
      }
    }
  }

  for (int i = 0; i < MAX_SNACKS; ++i) {
    if (!snacks[i].chomped) {
      map[snacks[i].y * MAP_WIDTH + snacks[i].x] = '*';
    }
  }

  for (int i = 0; i < snake->length; ++i) {
    map[snake->parts[i].y * MAP_WIDTH + snake->parts[i].x] =
        (i == 0) ? '0' : 'o';
  }
}

void *move_snake(void *args) {
  Client_data *client_data = (Client_data *)args;
  Snake *snake = &client_data->snake;
  bool game_end = false;

  while (!game_end) {
    pthread_mutex_lock(&mutex);
    for (int i = snake->length - 1; i > 0; --i) {
      snake->parts[i] = snake->parts[i - 1];
    }

    snake->parts[0].x += snake->dirX;
    snake->parts[0].y += snake->dirY;

    if (snake->parts[0].x <= 0 || snake->parts[0].x >= MAP_WIDTH - 1 ||
        snake->parts[0].y <= 0 || snake->parts[0].y >= MAP_HEIGHT - 1) {
      game_end = true;
    }

    for (int i = 1; i < snake->length; ++i) {
      if (snake->parts[0].x == snake->parts[i].x &&
          snake->parts[0].y == snake->parts[i].y) {
        game_end = true;
      }
    }

    for (int i = 0; i < MAX_SNACKS; ++i) {
      if (!snacks[i].chomped && snacks[i].x == snake->parts[0].x &&
          snacks[i].y == snake->parts[0].y) {
        snacks[i].chomped = true;
        snake->length++;
      }
    }

    generate_map(snake);
    send(client_data->client_socket, map, sizeof(map), 0);

    pthread_mutex_unlock(&mutex);
    usleep(SNAKE_SPEED);
  }

  close(client_data->client_socket);
  printf("Client %d disconnected.\n", client_data->id);
  free(client_data);
  return NULL;
}

void *handle_client(void *args) {
  Client_data *client_data = (Client_data *)args;
  Snake *snake = &client_data->snake;

  printf("Client %d connected.\n", client_data->id);

  pthread_t move_snake_thread;
  pthread_create(&move_snake_thread, NULL, move_snake, client_data);

  while (1) {
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
    default:
      break;
    }
    pthread_mutex_unlock(&mutex);

    usleep(SNAKE_SPEED);
  }

  pthread_cancel(move_snake_thread);
  pthread_join(move_snake_thread, NULL);

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

  if (listen(server_socket, 1) == -1) {
    perror("Listen failed!\n");
    return EXIT_FAILURE;
  }

  printf("Server is listening on port: %d\n", PORT);

  int client_id = 0;
  while (1) {
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
    spawn_snake(&client_data->snake);

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_client, client_data);
    pthread_detach(client_thread);
  }

  close(server_socket);
  pthread_mutex_destroy(&mutex);

  return EXIT_SUCCESS;
}
