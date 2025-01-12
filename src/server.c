#include "server.h"

// game settings
Game_data game = {
    .game_end = false,
    .timed_game = false,
    .obstacles = false,
    .game_running = false,
    .active_snakes = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .snake_speed = SNAKE_SPEED,
    .game_delay = TIMED_GAME_DELAY,
    .pause_delay = PAUSE_DELAY,
};

void move_snakes() {
  for (int s = 0; s < game.map.data.max_snakes; ++s) {
    // move snake
    Snake *snake = &game.map.data.snakes[s];
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
    if (snake->parts[0].x <= 0 || snake->parts[0].x >= game.map.map_width - 1 ||
        snake->parts[0].y <= 0 ||
        snake->parts[0].y >= game.map.map_height - 1) {
      snake->playing = false;
    }

    // obstacle collision
    for (int i = 0; i < game.map.data.obstacle_count; ++i) {
      if (game.map.data.obstacles[i].x == snake->parts[0].x &&
          game.map.data.obstacles[i].y == snake->parts[0].y) {
        snake->playing = false;
      }
    }

    // snake collisions
    for (int j = 0; j < game.map.data.max_snakes; ++j) {
      Snake *tmp_snake = &game.map.data.snakes[j];
      for (int i = 1; i < tmp_snake->length; ++i) {
        if (!tmp_snake->paused && snake->parts[0].x == tmp_snake->parts[i].x &&
            snake->parts[0].y == tmp_snake->parts[i].y) {
          snake->playing = false;
        }
      }
    }

    // snack collisions
    for (int i = 0; i < game.map.data.max_snacks; ++i) {
      if (!game.map.data.snacks[i].chomped &&
          game.map.data.snacks[i].x == snake->parts[0].x &&
          game.map.data.snacks[i].y == snake->parts[0].y) {
        game.map.data.snacks[i].chomped = true;
        snake->length++;
        spawn_snack(&game.map);
      }
    }
  }
}

void *game_loop(void *args) {
  int timer = 0;

  while (!game.game_end) {

    // move snakes and generate current map
    pthread_mutex_lock(&game.mutex);
    move_snakes();
    generate_map(&game.map);

    // send current map to active clients
    for (int i = 0; i < game.map.data.max_snakes; ++i) {
      if (game.client_sockets[i] != -1 && game.map.data.snakes[i].playing &&
          !game.map.data.snakes[i].paused) {
        int id = i;
        send(game.client_sockets[i], &id, sizeof(int), 0);

        int length = game.map.data.snakes[i].length;
        send(game.client_sockets[i], &length, sizeof(int), 0);

        send(game.client_sockets[i], game.map.map,
             game.map.map_width * game.map.map_height, 0);
      }
    }
    pthread_mutex_unlock(&game.mutex);

    // timed game logic
    if (game.timed_game) {
      ++timer;
      if (timer >= (game.game_delay * 1000000 / game.snake_speed)) {
        pthread_mutex_lock(&game.mutex);
        game.game_end = true;
        pthread_mutex_unlock(&game.mutex);
      }
    }

    // check if there are no active players
    if (game.game_running && game.active_snakes == 0) {
      game.game_end = true;
      break;
    }

    usleep(game.snake_speed);
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
    client_data->snake = &game.map.data.snakes[client_data->id];
    spawn_snake(client_data->snake, game.map.map_width, game.map.map_height);

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
  spawn_snack(&game.map);
  ++game.active_snakes;
  game.game_running = true;
  pthread_mutex_unlock(&game.mutex);

  // send map dimensions to client
  send(client_data->client_socket, &game.map.map_width, sizeof(int), 0);
  send(client_data->client_socket, &game.map.map_height, sizeof(int), 0);

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
      generate_obstacles(&game.map);
    }
  }

  // user is playing
  char buffer[1];
  int pause_timer = 0;
  while (snake->playing) {
    recv(client_data->client_socket, buffer, 1, 0);

    // pause
    if (buffer[0] == 'r') {
      pthread_mutex_lock(&game.mutex);
      snake->paused = false;
      pthread_mutex_unlock(&game.mutex);
    }

    // disable player
    if (buffer[0] == 'e') {
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
      printf("pause %d\n", pause_timer);
      if (pause_timer >= game.pause_delay) {
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

  // setup server socket
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    perror("Socket creation failed!\n");
    return EXIT_FAILURE;
  }

  // when server closes release port
  int opt = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);

  // bind server socket
  if (bind(server_socket, (struct sockaddr *)&server_address,
           sizeof(server_address)) == -1) {
    perror("Bind failed!\n");
    return EXIT_FAILURE;
  }

  // listen for clients
  if (listen(server_socket, game.map.data.max_snakes) == -1) {
    perror("Listen failed!\n");
    return EXIT_FAILURE;
  }

  printf("Server is listening on port: %d\n", PORT);

  // map size
  game.map.map_width = MAP_WIDTH;
  game.map.map_height = MAP_HEIGHT;

  game.map.map =
      malloc((game.map.map_width * game.map.map_height) * sizeof(char));
  game.client_sockets = malloc(game.map.data.max_snakes * sizeof(int));

  // additional game settings
  Map_data data = {
      .max_snacks = MAX_SNACKS,
      .max_snakes = MAX_PLAYERS,
      .obstacle_count = OBSTACLE_COUNT,
      .current_snack = 0,
  };
  data.snacks = malloc(data.max_snacks * sizeof(Snack));
  data.snakes = malloc(data.max_snakes * sizeof(Snake));
  data.obstacles = malloc(data.obstacle_count * sizeof(Obstacle));

  game.map.data = data;

  // start game
  pthread_t game_thread;
  pthread_create(&game_thread, NULL, game_loop, NULL);

  // start listening for clients
  pthread_t handle_clients_thread;
  pthread_create(&handle_clients_thread, NULL, accept_clients, &server_socket);

  while (1) {
    if (game.game_end) {
      pthread_cancel(handle_clients_thread);
      pthread_join(handle_clients_thread, NULL);
      break;
    }
  }

  printf("Server closed!\n");
  close(server_socket);
  pthread_mutex_destroy(&game.mutex);

  printf("Scores for this game were: \n");
  for (int i = 0; i < game.map.data.max_snakes; ++i) {
    if (!game.map.data.snakes[i].length) {
      continue;
    }
    printf("Player %d:  %d points.\n", i + 1,
           game.map.data.snakes[i].length - 1);
  }

  free(game.client_sockets);
  free(game.map.map);
  free(game.map.data.snacks);
  free(game.map.data.snakes);
  free(game.map.data.obstacles);

  return EXIT_SUCCESS;
}
