#pragma once

#include "obstacle.h"
#include "snack.h"
#include "snake.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct {
  Snack *snacks;
  Snake *snakes;
  Obstacle *obstacles;
  int obstacle_count;
  int current_snack;
  int max_snacks;
  int max_snakes;
} Map_data;

typedef struct {
  int map_width;
  int map_height;
  char *map;
  Map_data data;
} Map;

void generate_map(Map *map);
void generate_obstacles(Map *map);
void spawn_snack(Map *map);
