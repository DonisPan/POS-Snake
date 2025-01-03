#include <arpa/inet.h>
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

void *render_game(void *args) {
  int client_socket = *(int *)args;
  while (1) {
    pthread_mutex_lock(&mutex);
    recv(client_socket, map, sizeof(map), 0);

    printf("\033c");
    for (int y = 0; y < MAP_HEIGHT; ++y) {
      for (int x = 0; x < MAP_WIDTH; ++x) {
        printf("%c ", map[y * MAP_WIDTH + x]);
      }
      putchar('\n');
    }
    fflush(stdout);
    pthread_mutex_unlock(&mutex);
    usleep(SNAKE_SPEED);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  struct sockaddr_in server_address;
  char buffer[1];

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
    perror("Connection failed!");
    return EXIT_FAILURE;
  }

  printf("Connected to the server\n");
  pthread_t render_thread;
  pthread_create(&render_thread, NULL, render_game, &client_socket);

  while (1) {
    printf("Enter_direction (wasd): ");

    buffer[0] = getchar();
    getchar();

    if (buffer[0] == 'q') {
      break;
    }

    send(client_socket, buffer, 1, 0);
  }

  close(client_socket);
  // pthread_cancel(render_thread);
  pthread_join(render_thread, NULL);

  return EXIT_SUCCESS;
}
