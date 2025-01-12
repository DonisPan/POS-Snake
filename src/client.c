#include "client.h"

Client_data game = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .paused = false,
};

void *render_game(void *args) {
  int client_socket = *(int *)args;

  while (1) {
    if (!game.paused) {
      int id;
      recv(client_socket, &id, sizeof(int), 0);

      int length;
      recv(client_socket, &length, sizeof(int), 0);

      recv(client_socket, game.map.map,
           game.map.map_width * game.map.map_height, 0);

      clear();
      printw("Player %d score: %d", id + 1, length - 1);
      pthread_mutex_lock(&game.mutex);
      for (int y = 0; y < game.map.map_height; ++y) {
        for (int x = 0; x < game.map.map_width; ++x) {
          switch (game.map.map[y * game.map.map_width + x]) {
          case 'X':
            attron(COLOR_PAIR(1));
            break;
          case '*':
            attron(COLOR_PAIR(2));
            break;
          default:
            attron(COLOR_PAIR(3));
            break;
          }

          mvaddch(y + 1, x * 2 + 1, game.map.map[y * game.map.map_width + x]);
          mvaddch(y + 1, x * 2 + 2, ' ');
        }
      }
      pthread_mutex_unlock(&game.mutex);
      printw("\n");
      refresh();
    }
    usleep(33333);
  }
  return NULL;
}

void *handle_input(void *args) {
  int client_socket = *(int *)args;

  char buffer[1];
  while (1) {
    // send pressed key
    buffer[0] = getch();
    send(client_socket, buffer, 1, 0);

    if (buffer[0] == 'q') {
      pthread_mutex_lock(&game.mutex);
      game.paused = true;
      pthread_mutex_unlock(&game.mutex);
      break;
    }
  }
  return NULL;
}

void start_server() {
  pid_t pid = fork();
  if (pid == 0) {
    execl("./server", "./server", NULL);
  } else if (pid > 0) {
    usleep(1000000);
  } else {
    exit(EXIT_FAILURE);
  }
}

int connect_to_server(int client_sock) {
  struct sockaddr_in server_address;

  int client_socket = client_sock;
  if (client_sock == -1) {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == -1) {
      perror("Client socket wasn't created!\n");
      return EXIT_FAILURE;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_socket, (struct sockaddr *)&server_address,
                sizeof(server_address)) == -1) {
      // if client socket can't connect to server
      close(client_socket);
      perror("Couldn't connect to server!\n");
      return EXIT_FAILURE;
    }
  }

  // before game starts, setup map for rendering
  recv(client_socket, &game.map.map_width, sizeof(int), 0);
  recv(client_socket, &game.map.map_height, sizeof(int), 0);
  game.map.map =
      malloc(game.map.map_width * game.map.map_height * sizeof(char));

  sleep(1);
  return client_socket;
}

void render_menu() {
  clear();
  printw("Choose option:\n");
  printw("1. New Game\n");
  printw("2. Join Game\n");
  printw("3. Continue\n");
  printw("4. Exit\n");
  refresh();
}

void render_game_mode_menu() {
  clear();
  printw("Choose mode:\n");
  printw("1. Timed Game\n");
  printw("2. Unlimited Game\n");
  refresh();
}

void render_map_type_menu() {
  clear();
  printw("Choose map:\n");
  printw("1. With Obstacles\n");
  printw("2. Clean\n");
  refresh();
}

int main() {
  // ncurses setup
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  // ncurses colors setup
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);

  pthread_t render_thread;
  pthread_t input_thread;

  int client_socket = -1;
  char option = 0;
  while (option != '4') {
    clear();
    render_menu();
    option = getch();

    switch (option) {
      char option_buffer[1];

    // new game
    case '1':
      printw("Starting server...\n");
      refresh();
      start_server();
      client_socket = connect_to_server(-1);

      // select game mode
      while (1) {
        render_game_mode_menu();
        char mode = getch();
        if (mode == '1' || mode == '2') {
          send(client_socket, &mode, 1, 0);
          break;
        }
      }

      // select map type
      while (1) {
        render_map_type_menu();
        char mode = getch();
        if (mode == '1' || mode == '2') {
          send(client_socket, &mode, 1, 0);
          break;
        }
      }
      break;

    // join game
    case '2':
      printw("Joining game...\n");
      refresh();
      client_socket = connect_to_server(-1);
      if (client_socket == -1) {
        printw("No server found!\n");
        refresh();
        sleep(1);
      }
      break;

    // unpause game
    case '3':
      printw("Returning to game...\n");
      refresh();
      if (client_socket == -1) {
        printw("No game to return to!\n");
        refresh();
        sleep(1);
        break;
      }

      // send 'r' to server to unpause
      option_buffer[0] = 'r';
      send(client_socket, &option_buffer, 1, 0);
      sleep(1);

      // unpause in client
      pthread_mutex_lock(&game.mutex);
      game.paused = false;
      pthread_mutex_unlock(&game.mutex);
      break;

    case '4':
      clear();
      printw("Exiting...\n");
      pthread_cancel(input_thread);
      sleep(1);
      refresh();
      if (client_socket == -1) {
        break;
      }

      // send 'e' to server to disconnect client
      option_buffer[0] = 'e';
      send(client_socket, &option_buffer, 1, 0);
      client_socket = -1;
      break;

    default:
      printw("Wrong input!\n");
      refresh();
      continue;
      break;
    }

    // if the client is connected to server
    if (client_socket != -1) {
      printw("Connected to the server\n");
      refresh();

      pthread_create(&render_thread, NULL, render_game, &client_socket);

      pthread_mutex_lock(&game.mutex);
      game.paused = false;
      pthread_mutex_unlock(&game.mutex);

      pthread_create(&input_thread, NULL, handle_input, &client_socket);

      pthread_join(input_thread, NULL);

      pthread_cancel(render_thread);
      pthread_join(render_thread, NULL);
      endwin();
    }

    if (game.map.map != NULL && !game.paused) {
      free(game.map.map);
    }

    sleep(1);
  }

  clear();
  refresh();
  endwin();
  sleep(1);
  return EXIT_SUCCESS;
}
