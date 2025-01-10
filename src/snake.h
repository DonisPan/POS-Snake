#pragma once

#include "snake_bit.h"
#include <stdlib.h>
#include <stdbool.h>

#define SNAKE_MAX_LENGTH 250

typedef struct {
  Snake_bit parts[SNAKE_MAX_LENGTH];
  int length;
  int dirX;
  int dirY;
  bool paused;
  bool playing;
} Snake;
