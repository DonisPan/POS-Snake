#include <arpa/inet.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT 4606

#define MAP_WIDTH 25
#define MAP_HEIGHT 25
#define MAX_SNACKS 250
#define MAX_PLAYERS 10
#define SNAKE_MAX_LENGTH 250
#define SNAKE_SPEED 500000
#define GAME_TIME 10

bool game_end = false;
bool timed_game = false;

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
int current_snack = 0;
Snake snakes[MAX_PLAYERS];
char map[MAP_WIDTH * MAP_HEIGHT];
int client_sockets[MAX_PLAYERS];
int active_snakes = 0;

void spawn_snake(Snake *snake);
void spawn_snack();
void generate_map();
void move_snakes();
void broadcast_map();
void *handle_client(void *args);
void *game_loop(void *args);
int get_active_snakes();

void spawn_snake(Snake *snake) {
  snake->length = 1;
  snake->parts[0].x = 1 + rand() % (MAP_WIDTH - 2);
  snake->parts[0].y = 1 + rand() % (MAP_HEIGHT - 2);
  snake->dirX = 0;
  snake->dirY = 0;
  snake->playing = true;
}

int get_active_snakes() {
  int num = 0;
  for (int i = 0; i < MAX_PLAYERS; ++i) {
    if (snakes[i].playing) {
      ++num;
    }
  }
  return num;
}

void spawn_snack() {
  bool can_spawn = false;
  int x;
  int y;
  while (!can_spawn) {
    x = 1 + rand() % (MAP_WIDTH - 2);
    y = 1 + rand() % (MAP_HEIGHT - 2);

    if (map[y * MAP_WIDTH + x] == ' ') {
      can_spawn = true;
    }
  }
  snacks[current_snack].x = x;
  snacks[current_snack].y = y;
  snacks[current_snack].chomped = false;
  ++current_snack;
}

void generate_map() {
  // map border
  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      if (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) {
        map[y * MAP_WIDTH + x] = '#';
        // map[y * MAP_WIDTH + x] = get_active_snakes();
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
      --active_snakes;
    }

    // self collisions
    for (int i = 1; i < snake->length; ++i) {
      if (snake->parts[0].x == snake->parts[i].x &&
          snake->parts[0].y == snake->parts[i].y) {
        snake->playing = false;
        --active_snakes;
      }
    }

    // snack collisions
    for (int i = 0; i < MAX_SNACKS; ++i) {
      if (!snacks[i].chomped && snacks[i].x == snake->parts[0].x &&
          snacks[i].y == snake->parts[0].y) {
        snacks[i].chomped = true;
        snake->length++;
        spawn_snack();
      }
    }
  }
}

void *game_loop(void *args) {
  int timer = 0;
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

    if (timed_game) {
      ++timer;
      if (timer >= (GAME_TIME * 1000000 / SNAKE_SPEED)) {
        printw("Game ended!\n");
        refresh();
        game_end = true;
      }
    }

    usleep(SNAKE_SPEED);
  }
  return NULL;
}

void *handle_client(void *args) {
  Client_data *client_data = (Client_data *)args;
  Snake *snake = client_data->snake;

  printw("Client %d connected.\n", client_data->id);
  refresh();
  sleep(1);
  client_sockets[client_data->id] = client_data->client_socket;

  pthread_mutex_lock(&mutex);
  spawn_snack();
  ++active_snakes;
  pthread_mutex_unlock(&mutex);

  // first player game mode set
  if (client_data->id == 0) {
    char mode[1];
    recv(client_data->client_socket, mode, 1, 0);
    if (mode[0] == '1') {
      timed_game = true;
    }
  }

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
    case 'q':
      snake->playing = false;
      break;
    default:
      break;
    }
    pthread_mutex_unlock(&mutex);
  }

  close(client_data->client_socket);
  pthread_mutex_lock(&mutex);
  snake->playing = false;
  --active_snakes;
  pthread_mutex_unlock(&mutex);
  printf("Client %d disconnected.\n", client_data->id);
  free(client_data);

  return NULL;
}

int main(int argc, char *argv[]) {
  srand(time(NULL));

  // setup server socket
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Socket creation failed!\n");
    return EXIT_FAILURE;
  }

  int opt = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

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

  // start game
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

    printf("%d\n", get_active_snakes());
    if (get_active_snakes() == 0) {
      game_end = true;
    }
  }

  printf("Server closed!\n");
  close(server_socket);
  pthread_mutex_destroy(&mutex);

  return EXIT_SUCCESS;
}
