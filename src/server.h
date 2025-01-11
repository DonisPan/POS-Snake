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

// map dimensions
// #define MAP_WIDTH 25
// #define MAP_HEIGHT 25
// maximum snacks that can spawn in game
// #define MAX_SNACKS 250
// #define OBSTACLE_COUNT 20
// maximum of players that can join server
 #define MAX_PLAYERS 10
// #define SNAKE_MAX_LENGTH 250
// how often the snake moves
// #define SNAKE_SPEED 500000
// time limit of timed game
// #define GAME_TIME 10
// timeout after pausing snake
// #define PAUSE_TIME 5

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
  int client_sockets[MAX_PLAYERS];
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
