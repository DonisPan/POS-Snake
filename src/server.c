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

// map dimensions
#define MAP_WIDTH 25
#define MAP_HEIGHT 25
// maximum snacks that can spawn in game
#define MAX_SNACKS 250
#define OBSTACLE_COUNT 20
// maximum of players that can join server
#define MAX_PLAYERS 10
#define SNAKE_MAX_LENGTH 250
// how often the snake moves
#define SNAKE_SPEED 500000
// time limit of timed game
#define GAME_TIME 10
// timeout after pausing snake
#define PAUSE_TIME 5

typedef struct {
  int x;
  int y;
} Snake_bit;

typedef struct {
  Snake_bit parts[SNAKE_MAX_LENGTH];
  int length;
  int dirX;
  int dirY;
  bool paused;
  bool playing;
} Snake;

typedef struct {
  int x;
  int y;
  bool chomped;
} Snack;

typedef struct {
  int x;
  int y;
} Obstacle;

typedef struct {
  int id;
  int client_socket;
  Snake *snake;
} Client_data;

typedef struct {
  bool game_end;
  bool timed_game;
  bool obstacles;
  bool game_running;
  char map[MAP_WIDTH * MAP_HEIGHT];
  Snack snacks[MAX_SNACKS];
  Snake snakes[MAX_PLAYERS];
  Obstacle obstacle_map[OBSTACLE_COUNT];
  int client_sockets[MAX_PLAYERS];
  int current_snack;
  int active_snakes;
  pthread_mutex_t mutex;
} Game_data;

Game_data game = {
    .game_end = false,
    .timed_game = false,
    .obstacles = false,
    .game_running = false,
    .active_snakes = 0,
    .current_snack = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

void spawn_snake(Snake *snake);
void spawn_snack();
void generate_map();
void generate_obstacles();
void move_snakes();
void broadcast_map();
void *handle_client(void *args);
void *game_loop(void *args);

void spawn_snake(Snake *snake) {
  snake->length = 1;
  snake->parts[0].x = 1 + rand() % (MAP_WIDTH - 2);
  snake->parts[0].y = 1 + rand() % (MAP_HEIGHT - 2);
  snake->dirX = 0;
  snake->dirY = 0;
  snake->paused = false;
  snake->playing = true;
}

void spawn_snack() {
  bool can_spawn = false;
  int x;
  int y;

  while (!can_spawn) {
    x = 1 + rand() % (MAP_WIDTH - 2);
    y = 1 + rand() % (MAP_HEIGHT - 2);

    // check if the spot is empty
    if (game.map[y * MAP_WIDTH + x] == ' ') {
      can_spawn = true;
    }
  }

  // add created snack
  game.snacks[game.current_snack].x = x;
  game.snacks[game.current_snack].y = y;
  game.snacks[game.current_snack].chomped = false;
  ++game.current_snack;
}

void generate_obstacles() {
  int placed = 0;
  while (placed < OBSTACLE_COUNT) {
    int x = 1 + rand() % (MAP_WIDTH - 2);
    int y = 1 + rand() % (MAP_HEIGHT - 2);
    game.obstacle_map[placed].x = x;
    game.obstacle_map[placed].y = y;
    ++placed;
  }
}

void generate_map() {
  // map border
  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      if (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) {
        game.map[y * MAP_WIDTH + x] = '#';
      } else {
        game.map[y * MAP_WIDTH + x] = ' ';
      }
    }
  }

  // obstacles
  for (int i = 0; i < OBSTACLE_COUNT; ++i) {
    game.map[game.obstacle_map[i].y * MAP_WIDTH + game.obstacle_map[i].x] = 'X';
  }

  // snacks
  for (int i = 0; i < MAX_SNACKS; ++i) {
    if (!game.snacks[i].chomped) {
      game.map[game.snacks[i].y * MAP_WIDTH + game.snacks[i].x] = '*';
    }
  }

  // snakes
  for (int s = 0; s < MAX_PLAYERS; ++s) {
    Snake *snake = &game.snakes[s];
    if (!snake->playing || snake->paused) {
      continue;
    }
    for (int i = 0; i < snake->length; ++i) {
      game.map[snake->parts[i].y * MAP_WIDTH + snake->parts[i].x] =
          (i == 0) ? '0' : 'o';
    }
  }
}

void move_snakes() {
  for (int s = 0; s < MAX_PLAYERS; ++s) {
    // move snake
    Snake *snake = &game.snakes[s];
    if (!snake->playing || snake->paused ||
        (snake->dirX == 0 && snake->dirY == 0)) {
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

    // obstacle collision
    for (int i = 0; i < OBSTACLE_COUNT; ++i) {
      if (game.obstacle_map[i].x == snake->parts[0].x &&
          game.obstacle_map[i].y == snake->parts[0].y) {
        snake->playing = false;
      }
    }

    // snake collisions
    for (int j = 0; j < MAX_PLAYERS; ++j) {
      Snake *tmp_snake = &game.snakes[j];
      for (int i = 1; i < tmp_snake->length; ++i) {
        if (snake->parts[0].x == tmp_snake->parts[i].x &&
            snake->parts[0].y == tmp_snake->parts[i].y) {
          snake->playing = false;
        }
      }
    }

    // snack collisions
    for (int i = 0; i < MAX_SNACKS; ++i) {
      if (!game.snacks[i].chomped && game.snacks[i].x == snake->parts[0].x &&
          game.snacks[i].y == snake->parts[0].y) {
        game.snacks[i].chomped = true;
        snake->length++;
        spawn_snack();
      }
    }
  }
}

void *game_loop(void *args) {
  int timer = 0;

  while (!game.game_end) {
    pthread_mutex_lock(&game.mutex);
    move_snakes();
    generate_map();

    for (int i = 0; i < MAX_PLAYERS; ++i) {
      if (game.client_sockets[i] != -1 && game.snakes[i].playing) {
        int id = i;
        send(game.client_sockets[i], &id, sizeof(int), 0);

        int length = game.snakes[i].length;
        send(game.client_sockets[i], &length, sizeof(int), 0);

        send(game.client_sockets[i], game.map, sizeof(game.map), 0);
      }
    }
    pthread_mutex_unlock(&game.mutex);

    // timed game logic
    if (game.timed_game) {
      ++timer;
      if (timer >= (GAME_TIME * 1000000 / SNAKE_SPEED)) {
        pthread_mutex_lock(&game.mutex);
        game.game_end = true;
        pthread_mutex_unlock(&game.mutex);
      }
    }

    if (game.game_running && game.active_snakes == 0) {
      game.game_end = true;
      break;
    }

    usleep(SNAKE_SPEED);
  }
  printf("Game ended!\n");
  return NULL;
}

void *accept_clients(void *args) {
  int *server_socket = (int *)args;

  int client_id = 0;
  while (1) {
    struct sockaddr_in client_address;
    socklen_t client_length = sizeof(client_address);

    int client_socket = accept(
        *server_socket, (struct sockaddr *)&client_address, &client_length);
    if (client_socket == -1) {
      perror("Accept failure!\n");
      continue;
    }

    printf("Client %d connected.\n", client_id);

    // give client its data
    Client_data *client_data = malloc(sizeof(Client_data));
    client_data->id = client_id;
    ++client_id;
    client_data->client_socket = client_socket;
    client_data->snake = &game.snakes[client_data->id];
    spawn_snake(client_data->snake);

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_client, client_data);
    pthread_detach(client_thread);
  }

  return NULL;
}

void *handle_client(void *args) {
  Client_data *client_data = (Client_data *)args;
  Snake *snake = client_data->snake;

  game.client_sockets[client_data->id] = client_data->client_socket;

  pthread_mutex_lock(&game.mutex);
  spawn_snack();
  ++game.active_snakes;
  game.game_running = true;
  pthread_mutex_unlock(&game.mutex);

  // first player game mode set
  if (client_data->id == 0) {
    char mode[1];
    recv(client_data->client_socket, mode, 1, 0);
    if (mode[0] == '1') {
      game.timed_game = true;
    }
    recv(client_data->client_socket, mode, 1, 0);
    if (mode[0] == '1') {
      game.obstacles = true;
      generate_obstacles();
    }
  }

  // user is playing
  char buffer[1];
  int pause_timer = 0;
  // bool playing = true;
  while (snake->playing) {
    recv(client_data->client_socket, buffer, 1, 0);

    if (buffer[0] == 'r') {
      pthread_mutex_lock(&game.mutex);
      snake->paused = false;
      pthread_mutex_unlock(&game.mutex);
    }

    if (buffer[0] == 'e') {
      printf("Recieved 'e'.\n");
      snake->playing = false;
      break;
    }

    if (!snake->paused) {
      pause_timer = 0;
      pthread_mutex_lock(&game.mutex);
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
        snake->paused = true;
        snake->dirX = 0;
        snake->dirY = 0;
        break;

      default:
        break;
      }
      pthread_mutex_unlock(&game.mutex);
    }

    // pause timeout
    if (snake->paused) {
      ++pause_timer;
      if (pause_timer >= (PAUSE_TIME * 1000000)) {
        sleep(2);
        snake->playing = false;
      }
      sleep(1);
    }
  }

  --game.active_snakes;
  printf("Client %d disconnected. Current active players: %d.\n",
         client_data->id, game.active_snakes);
  close(client_data->client_socket);
  free(client_data);

  return NULL;
}

int main() {
  srand(time(NULL));

  // generate_obstacles();

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

  // start listening for clients
  pthread_t handle_clients_thread;
  pthread_create(&handle_clients_thread, NULL, accept_clients, &server_socket);

  while (1) {
    if (game.game_end) {
      pthread_cancel(handle_clients_thread);
      break;
    }
  }

  printf("Server closed!\n");
  close(server_socket);
  pthread_mutex_destroy(&game.mutex);

  return EXIT_SUCCESS;
}
