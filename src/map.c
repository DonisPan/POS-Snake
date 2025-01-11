#include "map.h"

void spawn_snack(Map *map) {
  bool can_spawn = false;
  int x;
  int y;

  while (!can_spawn) {
    x = 1 + rand() % (map->map_width - 2);
    y = 1 + rand() % (map->map_height - 2);

    // check if the spot is empty
    if (map->map[y * map->map_width + x] == ' ') {
      can_spawn = true;
    }
  }

  // add created snack
  map->data.snacks[map->data.current_snack].x = x;
  map->data.snacks[map->data.current_snack].y = y;
  map->data.snacks[map->data.current_snack].chomped = false;
  ++map->data.current_snack;
}

void generate_obstacles(Map *map) {
  int placed = 0;
  while (placed < map->data.obstacle_count) {
    int x = 1 + rand() % (map->map_width - 2);
    int y = 1 + rand() % (map->map_height - 2);
    map->data.obstacles[placed].x = x;
    map->data.obstacles[placed].y = y;
    ++placed;
  }
}

void generate_map(Map *map) {
  // map border
  for (int y = 0; y < map->map_height; ++y) {
    for (int x = 0; x < map->map_width; ++x) {
      if (x == 0 || y == 0 || x == map->map_width - 1 ||
          y == map->map_height - 1) {
        map->map[y * map->map_width + x] = '#';
      } else {
        map->map[y * map->map_width + x] = ' ';
      }
    }
  }

  // obstacles
  for (int i = 0; i < map->data.obstacle_count; ++i) {
    map->map[map->data.obstacles[i].y * map->map_width +
             map->data.obstacles[i].x] = 'X';
  }

  // snacks
  for (int i = 0; i < map->data.max_snacks; ++i) {
    if (!map->data.snacks[i].chomped) {
      map->map[map->data.snacks[i].y * map->map_width + map->data.snacks[i].x] =
          '*';
    }
  }

  // snakes
  for (int s = 0; s < map->data.max_snakes; ++s) {
    Snake *snake = &map->data.snakes[s];
    if (!snake->playing || snake->paused) {
      continue;
    }
    for (int i = 0; i < snake->length; ++i) {
      map->map[snake->parts[i].y * map->map_width + snake->parts[i].x] =
          (i == 0) ? '0' : 'o';
    }
  }
  map->map[0] = '#';
}
