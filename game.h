#include <Arduino.h>

#ifndef __PCP_GAME_H
#define __PCP_GAME_H

#define GAME_MAX_BALLS 3

typedef struct {
  void (*setup)();
  void (*start_ball)();
  void (*loop)();
  void (*end_ball)();
} game_def;

void game_setup(game_def*);
void game_loop(void);
void game_add_score(unsigned long long);
void launch_ball(byte num);
void set_extra_ball(void);
void activate_output(byte);

#endif
