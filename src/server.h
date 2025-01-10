#pragma once

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

void spawn_snake(Snake *snake);
void spawn_snack();
void generate_map();
void generate_obstacles();
void move_snakes();
void broadcast_map();
void *handle_client(void *args);
void *game_loop(void *args);
