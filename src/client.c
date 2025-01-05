#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 4606
#define BUFFER_SIZE 1024
#define MAP_WIDTH 25
#define MAP_HEIGHT 25
#define SNAKE_SPEED 500000

char map[MAP_WIDTH * MAP_HEIGHT];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void render_menu();
void render_game_mode_menu();

void *render_game(void *args);
void start_server();
int connect_to_server();

void *render_game(void *args) {
  int client_socket = *(int *)args;

  while (1) {
    pthread_mutex_lock(&mutex);
    recv(client_socket, map, sizeof(map), 0);

    clear();
    for (int y = 0; y < MAP_HEIGHT; ++y) {
      for (int x = 0; x < MAP_WIDTH; ++x) {
        mvaddch(y + 1, x * 2 + 1, map[y * MAP_WIDTH + x]);
        mvaddch(y + 1, x * 2 + 2, ' ');
      }
    }
    printw("\n");
    refresh();

    pthread_mutex_unlock(&mutex);
    usleep(SNAKE_SPEED);
  }
  endwin();
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

int connect_to_server() {
  struct sockaddr_in server_address;

  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    perror("Socket creation failed!\n");
    return EXIT_FAILURE;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;

  if (connect(client_socket, (struct sockaddr *)&server_address,
              sizeof(server_address)) == -1) {
    close(client_socket);
    perror("Connection failed!\n");
    return -1;
  }
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

int main(int argc, char *argv[]) {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  int client_socket = -1;
  char option = 0;
  while (option != '4') {
    render_menu();
    option = getch();

    switch (option) {
    case '1':
      printw("Starting server...\n");
      refresh();
      start_server();
      client_socket = connect_to_server();

      // game mode
      while (1) {
        render_game_mode_menu();
        char mode = getch();
        if (mode == '1' || mode == '2') {
          send(client_socket, &mode, 1, 0);
          break;
        }
      }

      break;

    case '2':
      printw("Joining game...\n");
      refresh();
      client_socket = connect_to_server();
      if (client_socket == -1) {
        printw("No server found!\n");
        refresh();
        sleep(2);
        endwin();
        // exit(EXIT_FAILURE);
      }

      break;

    case '3':
      printw("Returning to game...\n");
      refresh();
      endwin();
      break;

    default:
      printw("Wrong input!\n");
      refresh();
      break;
    }

    if (client_socket != -1) {
      printw("Connected to the server\n");
      refresh();

      pthread_t render_thread;
      pthread_create(&render_thread, NULL, render_game, &client_socket);

      char buffer[1];
      buffer[0] = ' ';
      while (buffer[0] != 'q') {
        buffer[0] = getch();

        if (buffer[0] == 'q') {
          pthread_mutex_lock(&mutex);
          printw("Client has exited the game.\n");
          refresh();
          sleep(2);
          pthread_mutex_unlock(&mutex);
        }

        send(client_socket, buffer, 1, 0);
      }

      close(client_socket);
      pthread_cancel(render_thread);
      pthread_join(render_thread, NULL);
      client_socket = -1;
    }
  }
  endwin();
  return EXIT_SUCCESS;
}
