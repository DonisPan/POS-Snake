#include <stdio.h>
#include <stdlib.h>
// #include <conio.h> //DONT KNOW WHAT IT DOES YET
#include <stdbool.h>
#include <time.h>

#define map_width 25
#define map_height 25
#define max_snacks 50

char map[map_width * map_height];

bool game_end = false;

void generate_map() {
  for (int y = 0; y < map_height; ++y) {
    for (int x = 0; x < map_width; ++x) {
      if (x == 0 || y == 0 || x == map_width - 1 || y == map_height - 1) {
        map[y * map_width + x] = '#';
      } else {
        map[y * map_width + x] = ' ';
      }
    }
  }
}

void clear_screen() { system("clear"); }

void render_map() {
  for (int y = 0; y < map_height; ++y) {
    for (int x = 0; x < map_width; ++x) {
      putchar(map[y * map_width + x]);
    }
    putchar('\n');
  }
}

#define SNAKE_MAX_LENGTH 256
typedef struct {
  int x;
  int y;
} Snake_bit;

typedef struct {
  int length;
  Snake_bit parts[SNAKE_MAX_LENGTH];
} Snake;

Snake snake;

typedef struct {
  int x;
  int y;
  bool chomped;
} Snack;

Snack snacks[max_snacks];

void render_snake() {
  for(int i = snake.length - 1; i > 0; --i) {
    map[snake.parts[i].y*map_width + snake.parts[i].x] = 'o';
  }
 map[snake.parts[0].y*map_width + snake.parts[0].x] = '0'; 
}

void move_snake(int deltaX, int deltaY) {
  for(int i = snake.length -1; i > 0; --i) {
    snake.parts[i] = snake.parts[i -1];
  }

  snake.parts[0].x += deltaX;
  snake.parts[0].y += deltaY;
}

void read_keyboard_input() {
  int key = getchar();

  switch(key) {
    case 'w' : move_snake(0, -1); break;
    case 's' : move_snake(0, 1); break;
    case 'a' : move_snake(-1, 0); break;
    case 'd' : move_snake(1, 0); break;
  }
}

void render_snacks() {
  for(int i = 0; i < max_snacks; ++i) {
    if(!snacks[i].chomped) {
      map[snacks[i].y*map_width + snacks[i].x] = '~';
    }
  }
}

void spawn_snacks() {
  for(int i = 0; i < max_snacks; ++i) {
    snacks[i].x = 1 + rand() % (map_width -2);
    snacks[i].y = 1 + rand() % (map_height -2);
    snacks[i].chomped = false;
  }
}

void spawn_snake() {
  snake.length = 1;
  snake.parts[0].x = 1 + rand() % (map_width -2);
  snake.parts[0].y = 1 + rand() % (map_height -2);
}

void game_rules() {
//EAT FOOD
  for(int i = 0; i < max_snacks; ++i) {
    if(!snacks[i].chomped) {
      if(snacks[i].x == snake.parts[0].x && snacks[i].y == snake.parts[0].y) {
        snacks[i].chomped = true;
        snake.length++;
      }
    }
  }

//END GAME
  if(snake.parts[0].x == 0 || snake.parts[0].x == map_width -1 ||
      snake.parts[0].y == 0 || snake.parts[0].y == map_height -1) {
    game_end = true;
  }

  for(int i = 1; i < snake.length; ++i) {
    if(snake.parts[0].x == snake.parts[i].x && snake.parts[0].y == snake.parts[i].y) {
      game_end = true;
    }
  }
}

int main(int argc, char *argv[])
{
  srand(time(NULL));

  spawn_snake();
  spawn_snacks();

  while(!game_end) {
    generate_map();
    render_snacks();
    render_snake();
    game_rules();
    clear_screen();
    printf("Score: %d\n", snake.length);
    render_map();
    if(!game_end) {
      read_keyboard_input();
    }
  }

  printf("Game end, Final score: %d\n", snake.length);
  return EXIT_SUCCESS;
}
