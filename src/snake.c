#include "snake.h"

void spawn_snake(Snake *snake, int max_width, int max_height) {
  snake->length = 1;
  snake->parts[0].x = 1 + rand() % (max_width - 2);
  snake->parts[0].y = 1 + rand() % (max_height - 2);
  snake->dirX = 0;
  snake->dirY = 0;
  snake->paused = false;
  snake->playing = true;
}
