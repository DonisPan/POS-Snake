#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 1234
#define BUFFER_SIZE 1024

#define MAP_WIDTH 25
#define MAP_HEIGHT 25

#define SNAKE_SPEED 500000

char map[MAP_WIDTH * MAP_HEIGHT];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *render_game(void *args);
void start_server();
int connect_to_server();

void *render_game(void *args) {
  int client_socket = *(int *)args;
  while (1) {
    pthread_mutex_lock(&mutex);
    recv(client_socket, map, sizeof(map), 0);

    // printf("\033c");
    //  for (int y = 0; y < MAP_HEIGHT; ++y) {
    //    for (int x = 0; x < MAP_WIDTH; ++x) {
    //      printf("%c ", map[y * MAP_WIDTH + x]);
    //    }
    //    putchar('\n');
    //  }
    //  fflush(stdout);

    clear();
    for (int y = 0; y < MAP_HEIGHT; ++y) {
      for (int x = 0; x < MAP_WIDTH; ++x) {
        mvaddch(y, x, map[y * MAP_WIDTH + x]);
      }
    }
    refresh();

    pthread_mutex_unlock(&mutex);
    usleep(SNAKE_SPEED);
  }
  return NULL;
}

void start_server() {
  pid_t pid = fork();
  if (pid == 0) {
    execl("./server", "./server", NULL);
  } else {
    usleep(1000000);
  }
}

int connect_to_server() {
  struct sockaddr_in server_address;
  // char buffer[1];

  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    perror("Socket creation failed!");
    return EXIT_FAILURE;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  server_address.sin_addr.s_addr = INADDR_ANY;

  if (connect(client_socket, (struct sockaddr *)&server_address,
              sizeof(server_address)) == -1) {
    close(client_socket);
    perror("Connection failed!");
    return -1;
  }
  return client_socket;
}

int main(int argc, char *argv[]) {
  // struct sockaddr_in server_address;

  // int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  // if (client_socket == -1) {
  //   perror("Socket creation failed!");
  //   return EXIT_FAILURE;
  // }

  // server_address.sin_family = AF_INET;
  // server_address.sin_port = htons(PORT);
  // server_address.sin_addr.s_addr = INADDR_ANY;

  // if (connect(client_socket, (struct sockaddr *)&server_address,
  //             sizeof(server_address)) == -1) {
  //   perror("Connection failed!");
  //   return EXIT_FAILURE;
  // }

  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);

  int client_socket = connect_to_server();

  if (client_socket == -1) {
    printw("Starting server...");
    refresh();
    start_server();
    client_socket = connect_to_server();
  }

  printw("Connected to the server\n");
  refresh();

  pthread_t render_thread;
  pthread_create(&render_thread, NULL, render_game, &client_socket);

  while (1) {
    char buffer[1];
    printw("Enter_direction (wasd): ");
    refresh();

    buffer[0] = getch();
    // getchar();

    if (buffer[0] == 'q') {
      break;
    }

    send(client_socket, buffer, 1, 0);
  }


  close(client_socket);
  pthread_cancel(render_thread);
  pthread_join(render_thread, NULL);
  endwin();
  return EXIT_SUCCESS;
}
