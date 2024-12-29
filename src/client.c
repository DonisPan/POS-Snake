#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 1234
#define BUFFER_SIZE 1024

#define MAP_WIDTH 25
#define MAP_HEIGHT 25

void render_game(char *map) {
  //printf("\003[H\033[J");
  system("clear");
  for (int y = 0; y < MAP_HEIGHT; ++y) {
    for (int x = 0; x < MAP_WIDTH; ++x) {
      putchar(map[y * MAP_WIDTH + x]);
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  int client_socket;
  struct sockaddr_in server_address;
  char buffer[BUFFER_SIZE];
  char map[MAP_WIDTH * MAP_HEIGHT];

  client_socket = socket(AF_INET, SOCK_STREAM, 0);
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

  while (1) {
    printf("Enter_direction (wasd): ");
    char input = getchar();
    getchar();
    buffer[0] = input;

    send(client_socket, buffer, 1, 0);

    memset(map, 0, sizeof(map));
    recv(client_socket, map, sizeof(map), 0);

    render_game(map);
  }

  close(client_socket);

  return EXIT_SUCCESS;
}
