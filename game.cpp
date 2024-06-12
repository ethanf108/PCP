#include "game.h"
#include "log.h"
#include "sensor.h"
#include "output.h"
#include "task_list.h"
#include <Arduino.h>


static game_def* game = NULL;
static TaskList* task_list = new TaskList;

static enum {
  NONE,           // Waiting, no game etc
  ADDING_PLAYERS, // Start button pressed
  GAME,           // playing game
  END_BALL,       // Waiting for all balls to return
} game_state = NONE;

static unsigned long long player_scores[4] = {0ULL, 0ULL, 0ULL, 0ULL};
static byte number_of_players = 0;
static int8_t current_player = -1;
static byte ball_num = -1;

static byte number_of_tilts = 0;
static unsigned long ball_save_timer = 0;
static boolean extra_ball = false;

static unsigned long long activate_outputs = 0;
static byte queued_balls = 0;



// Returns true if this is the first time
// this function has been run during this state
static boolean do_once() {
  static typeof game_state last = END_BALL; // Not clean :(

  boolean ret = last != game_state;
  last = game_state;
  
  loggf(TRACE, "do_once called with return value %s", ret ? "YES" : "NO");
  
  return ret;
}

// Depending on sensors, update things like
// Flippers, slings, pop bumpers, etc
// if clear flag is set, turn off all outputs
static void update_common_outputs(boolean clear) {
  loggf(TRACE, "Common outputs updating %s", clear ? " (cleared)" : "");
  
  OUTPUT_flip1(!clear && SENSOR_fl1);
  OUTPUT_flip2(!clear && SENSOR_fl2);

  static unsigned long times[10] = {0};

  static byte on[11] = {0};
  int index = 0;

  static byte waiting_for_kick = 0;
  static unsigned long kick_timeout = 0;

  const unsigned long now = millis();

  // My finest work
#define SAFE(s, o, i)							\
  if (now >= times[i] && now - times[i] >= SAFE_OUTPUT_COOLDOWN && ((s) || \
								    (activate_outputs & (1ULL << o)))) { \
    times[i] = now;							\
    on[index++] = o;							\
    activate_outputs &= ~(1ULL << o);					\
    if (o == OUTPUT_kick && waiting_for_kick == 2) {			\
      kick_timeout = now + 250;						\
      waiting_for_kick = 0;						\
      queued_balls--;							\
    }									\
  }

  if (clear) {
    activate_outputs = 0; // TODO is this a good idea?
  } else {
    if (queued_balls > 0 && waiting_for_kick == 0 && now > kick_timeout) {
      waiting_for_kick = 1;
      activate_output(OUTPUT_trough);
    } else if (waiting_for_kick == 1 && SENSOR_kick) {
      times[6] = now + 250;
      waiting_for_kick = 2;
      activate_output(OUTPUT_kick);
    }
    
    SAFE(SENSOR_sling1, OUTPUT_sling1, 0);
    SAFE(SENSOR_sling2, OUTPUT_sling2, 1);
    SAFE(SENSOR_pop1, OUTPUT_pop1, 2);
    SAFE(SENSOR_pop2, OUTPUT_pop2, 3);
    SAFE(SENSOR_pop3, OUTPUT_pop3, 4);
    if (SENSOR_trough_ready || true) { // TODO figure out trough sensor
      SAFE(false, OUTPUT_trough, 5);
    }
    SAFE(false, OUTPUT_kick, 6);
    SAFE(false, OUTPUT_drop1, 7);
    SAFE(false, OUTPUT_drop2, 8);
    SAFE(false, OUTPUT_scoop, 9);
  }

  on[index] = 0;
  safe_outputs(on);  
}


void game_setup(game_def* g) {
  game = g;
  logg(INFO, "Game setup completed");
}

void game_loop() {
  static boolean start_latch = false;
  static boolean all_balls_returned = true;
  
  task_list->callTasks();

  //loggf(DEBUG, "GAME STATE: %u", game_state);
  
  switch (game_state) {    
  case NONE: {
    if (do_once()) {
      for(byte i = 0; i < 4; i++) {
	player_scores[i] = 0ULL;
      }
      
      number_of_players = 0;
      current_player = -1;
      ball_num = -1;
      number_of_tilts = 0;
      ball_save_timer = 0;
      extra_ball = false;
      queued_balls = 0;
      activate_outputs = 0;
    
      update_common_outputs(true);
      
      game->setup();
    }

    if (SENSOR_start) {
      start_latch = true;
      logg(INFO, "First player added");
      game_state = ADDING_PLAYERS;
      number_of_players = 1;
    }
    
    break;
  }
    
  case ADDING_PLAYERS: {
    if (do_once()) {
      activate_output(OUTPUT_scoop);
      activate_output(OUTPUT_kick);
    }
    
    update_common_outputs(false);
    
    if (SENSOR_start && !start_latch && number_of_players < 4) {
      start_latch = true;
      number_of_players++;
      loggf(INFO, "Player added: %u", number_of_players);
    } else if(!SENSOR_start) {
      start_latch = false;
    }

    if (SENSOR_fl1) { // TODO add back in fl2, it's broken rn (leo)
      game_state = GAME;
      launch_ball(1);
    }
    
    break;
  }

  case GAME: {
    if (do_once()) {
      current_player = (current_player + 1) % number_of_players;
      if (current_player == 0) {
	ball_num++;
	if (ball_num >= GAME_MAX_BALLS) {
	  logg(INFO, "Game End");
	  // TODO proper game end ?
	  game_state = NONE;
	  break;
	}
      }
      
      activate_output(OUTPUT_drop1);
      activate_output(OUTPUT_drop2);

      game->start_ball();
      
      loggf(INFO, "Player %d starting", current_player + 1);
    }
    
    // If ball ends
    if (trough_count() == 8 && queued_balls == 0) {
      // Ten seconds
      // TODO move to garret_game
      if (millis() - ball_save_timer <= 10000) {
	logg(INFO, "Ball saved");
	launch_ball(1);
	ball_save_timer = 0;
      } else {
	logg(INFO, "Ball ended");
	game_state = END_BALL;
	break;
      }
    }
    
    if (LATCH_tilt) {
      logg(TRACE, "Tilt detected");
      number_of_tilts++;
      if (number_of_tilts >= 3) {
	logg(INFO, "TILT!! lol. Ending ball");
	game_state = END_BALL;
	break;
      }
    }

    update_common_outputs(false);

    game->loop();
    
    break;
  }
    
  case END_BALL: {
    if (do_once()) {
      update_common_outputs(true);
      all_balls_returned = false;
      game->end_ball();
    }
    
    if (trough_count() == 8 && !all_balls_returned) {
      logg(INFO, "All balls returned to trough, starting new ball");
      all_balls_returned = true;
      
      if (extra_ball) {
	logg(INFO, "Extra ball present, same player");
	extra_ball = false;
      }

      launch_ball(1);
    }

    if (all_balls_returned && trough_count() < 8) {
      game_state = GAME;
    }
    
    break;
  }
    
  default: {
    logg(ERROR, "Lol what happened here");
    // OOPS !
  }
  }
}

void game_add_score(unsigned long long score) {
  if (score == 0) {
    logg(TRACE, "Zero score added");
    return;
  }
  loggf(DEBUG, "Score added: %lu", score);
  player_scores[current_player] += score;
  loggf(DEBUG, "Score total is now %lu", player_scores[current_player]);
}

// Launch a ball into the lane
// If kick is true, also kick the ball
void launch_ball(byte num) {
  loggf(DEBUG, "Added %u balls", num);
  queued_balls += num;
}

void set_extra_ball() {
  logg(TRACE, "Set extra ball flag");
  extra_ball = true;
}

void activate_output(byte output) {
  activate_outputs |= (1ULL << output);
}
