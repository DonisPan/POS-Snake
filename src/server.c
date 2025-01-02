#include <arpa/inet.h>
#include <pthread.h>
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
#define SNAKE_SPEED 500000

typedef struct {
  int x;
  int y;
} Snake_bit;

typedef struct {
  Snake_bit parts[SNAKE_MAX_LENGTH];
  int length;
  int deltaX;
  int deltaY;
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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// typedef struct {
//   int client_socket;
// } ClientData;

void spawn_snake();
void spawn_snacks();
// void move_snake(int deltaX, int deltaY);
void game_rules();
void generate_map();
void *handle_client(void *args);
void *move_snake(void *args);

void spawn_snake() {
  snake.length = 1;
  // snake.parts[0].x = 1 + rand() % (MAP_WIDTH - 2);
  // snake.parts[0].y = 1 + rand() % (MAP_HEIGHT - 2);
  snake.parts[0].x = 1;
  snake.parts[0].y = 1;
  snake.deltaX = 1;
  snake.deltaY = 0;
}

void spawn_snacks() {
  for (int i = 0; i < MAX_SNACKS; ++i) {
    snacks[i].x = 1 + rand() % (MAP_WIDTH - 2);
    snacks[i].y = 1 + rand() % (MAP_HEIGHT - 2);
    snacks[i].chomped = false;
  }
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
  // memset(map, ' ', sizeof(map));

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

  for (int i = 0; i < snake.length; ++i) {
    map[snake.parts[i].y * MAP_WIDTH + snake.parts[i].x] = (i == 0) ? '0' : 'o';
  }
}

void *move_snake(void *args) {
  int client_socket = *(int *)args;

  while (!game_end) {
    pthread_mutex_lock(&mutex);

    for (int i = snake.length - 1; i > 0; --i) {
      snake.parts[i] = snake.parts[i - 1];
    }

    snake.parts[0].x += snake.deltaX;
    snake.parts[0].y += snake.deltaY;

    game_rules();
    generate_map();

    send(client_socket, map, sizeof(map), 0);
    pthread_mutex_unlock(&mutex);

    usleep(SNAKE_SPEED);
  }
  close(client_socket);
  return NULL;
}

void *handle_client(void *args) {

  // ClientData *client_data = (ClientData *)args;
  // int client_socket = client_data->client_socket;
  int client_socket = *(int *)args;

  pthread_mutex_lock(&mutex);
  printf("Sending initial map: \n");
  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      putchar(map[y * MAP_WIDTH + x]);
    }
    putchar('\n');
  }
  // send(client_socket, map, sizeof(map), 0);
  pthread_mutex_unlock(&mutex);

  while (!game_end) {
    char buffer[1];

    recv(client_socket, buffer, 1, 0);

    pthread_mutex_lock(&mutex);

    switch (buffer[0]) {
    case 'w':
      snake.deltaX = 0;
      snake.deltaY = -1;
      break;
    case 's':
      snake.deltaX = 0;
      snake.deltaY = 1;
      break;
    case 'a':
      snake.deltaX = -1;
      snake.deltaY = 0;
      break;
    case 'd':
      snake.deltaX = 1;
      snake.deltaY = 0;
      break;
    default:
      break;
    }

    // game_rules();
    // generate_map();

    // send(client_socket, map, sizeof(map), 0);

    pthread_mutex_unlock(&mutex);
  }

  close(client_socket);
  // free(client_data);

  return NULL;
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  // spawn_snake();
  spawn_snacks();

  generate_map();

  int server_socket;
  // int client_socket;

  struct sockaddr_in server_address;
  // struct sockaddr_in client_address;
  // socklen_t client_len = sizeof(client_address);

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

  while (!game_end) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(client_address);

    int client_socket = accept(
        server_socket, (struct sockaddr *)&client_address, &client_length);
    if (client_socket == -1) {
      perror("Accept failed!");
      return EXIT_FAILURE;
    }

    printf("Client connected\n");
    spawn_snake();
    pthread_t move_snakes;
    pthread_create(&move_snakes, NULL, move_snake, &client_socket);

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_client, &client_socket);
    pthread_detach(client_thread);

    // handle_client(client_socket);
  }

  close(server_socket);
  pthread_mutex_destroy(&mutex);

  return EXIT_SUCCESS;
}
