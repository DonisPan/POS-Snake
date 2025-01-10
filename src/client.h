#pragma once

#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 4606

#define BUFFER_SIZE 200;
#define MAP_WIDTH 25
#define MAP_HEIGHT 25
#define SNAKE_SPEED 500000

typedef struct {
  char map[MAP_WIDTH * MAP_HEIGHT];
  pthread_mutex_t mutex;
  bool paused;
} Client_data;

// print menus
void render_menu();
void render_game_mode_menu();
void render_map_type_menu();

// connection methods
void start_server();
int connect_to_server();

// game threads
void *render_game(void *args);
void *handle_input(void *args);


