#pragma once

#include "map.h"

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

// setings
#define MAP_HEIGHT 40
#define MAP_WIDTH 30
#define MAX_SNACKS 200
#define MAX_PLAYERS 10
#define OBSTACLE_COUNT 16
#define TIMED_GAME_DELAY 10
#define PAUSE_DELAY 5
#define SNAKE_SPEED 400000

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
  int *client_sockets;
  int active_snakes;
  int snake_speed;
  int max_snake_length;
  int game_delay;
  int pause_delay;
  pthread_mutex_t mutex;
  Map map;
} Game_data;

void move_snakes();
void *handle_client(void *args);
void *game_loop(void *args);
