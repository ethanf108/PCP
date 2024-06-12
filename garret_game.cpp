#include "garret_game.h"
#include "log.h"
#include "game.h"
#include "sensor.h"
#include "output.h"
#include <Arduino.h>

// Temporary main game implementation per Garret's rules

typedef enum: byte {
  MODE_NONE = 0,
  YOU_DID_WHAT = 1,
  OPCOMMATHON = 2,
  HOUSE_MEETING = 4,
  FINALS = 8,
} Mode;

static byte available_modes = MODE_NONE; // Flag set for all modes that have had their requirements met
static Mode current_mode = MODE_NONE; // Only one mode at a time, the current active mode
static byte completed_modes = MODE_NONE; // All modes that have been completed, flag set
static unsigned long mode_finish_timer = 0;
static boolean game_won = false;

static boolean mystery_available = false;
static boolean extra_ball_available = false;

typedef enum: byte {
  MULTI_NONE = 0,
  BAGELS = 1,
  PARTY_BUTTON = 2,
  PCP = 4,
} Multiball;

static byte available_multiballs = 0;
static Multiball current_multiball = MULTI_NONE;
static byte finished_multiballs = 0;

static byte PCP_phase = 0;
static byte PCP_pop_bumper_hits = 0;
static byte party_button_multi_hit_count = 0;

// Bonus counters
static unsigned int drinks = 0;
static unsigned int devcade_games = 0;
static unsigned int imagine_projects = 0;
static unsigned int bonus_multiplier = 1;

// Bits that signify if the lights for the CSH rollover are set
// format: 0b000oCSHo, CSH are the lights
// o are overflow bits for temp math.
static byte CSH_rollover_bits = 0;

// Same as above but 4 status bits (0b00o1976o)
static byte rollover_1976_bits = 0;

// Would be function static if I didn't have to clear them
static byte pop_bumper_hits = 0;
static byte vader_lobby_hit_count_left = 0;
static byte vader_lobby_hit_count_right = 0;
static byte rtp_order = 0; // sequential, 3 is all in order
static byte server_room_order = 0;
static byte mystery_count = 1; // Usually takes 2 hits, but will only take 1 the first time
static byte party_button_hit_count = 0;
static byte lounge_rect_counter = 0;
static byte elevator_count = 0;
static boolean kick_scoop = false;
static unsigned long kick_scoop_timer = 0;

// TODO clear above when done
// TODO create start_ball func

void garret_game_setup() {
  drinks = 0;
  devcade_games = 0;
  imagine_projects = 0;
  bonus_multiplier = 1;
  
  available_modes = MODE_NONE;
  current_mode = MODE_NONE;
  completed_modes = MODE_NONE;
  mode_finish_timer = 0;
  game_won = false;

  mystery_available = false;
  extra_ball_available = false;
  
  available_multiballs = 0;
  current_multiball = MULTI_NONE;
  finished_multiballs = 0;
  
  randomSeed(millis());
  CSH_rollover_bits = 1 << (random(2) + 1);
  rollover_1976_bits = 0;

  logg(TRACE, "Garret game setup complete");
}

void garret_game_start_ball() {
  logg(TRACE, "Garret Game Start Ball finished");
}

static void activate_multiball(Multiball mb) {
  if (current_multiball != MULTI_NONE) {
    return;
  }

  current_multiball = mb;

  switch (current_multiball) {
  case BAGELS: {
    launch_ball(2);
    break;
  }

  case PARTY_BUTTON: {
    launch_ball(7);
    party_button_multi_hit_count = 0;
    break;
  }

  case PCP: {
    PCP_phase = 0;
    PCP_pop_bumper_hits = 0;
    activate_output(OUTPUT_drop1);
    activate_output(OUTPUT_drop2);
    launch_ball(1);
    break;
  }
  }
}

static void pop_bumper() {
  logg(TRACE, "Pop bumper");

  if (current_multiball == PCP && PCP_phase == 1) {
    game_add_score(2000);
    drinks++;
    PCP_pop_bumper_hits++;

    if (PCP_pop_bumper_hits >= 60) {
      extra_ball_available = true;
      PCP_phase = 0;
      activate_output(OUTPUT_drop1);
      activate_output(OUTPUT_drop2);
    }

    return;
  }
  
  if (pop_bumper_hits >= 50) {
    game_add_score(20000);
    drinks += 5;
  } else {
    game_add_score(500);
    drinks++;
    pop_bumper_hits++;  
  }
}

static void slingshot() {
  logg(DEBUG, "Slingshot");
  
  game_add_score(200);
  drinks++;
}

// roll = 0 is right
static void CSH_rollover(byte roll) {
  loggf(TRACE, "C-S-H Rollover %u", roll);
  
  game_add_score(1000);
  drinks++;
  CSH_rollover_bits |= 1 << (roll + 1);

  if ((CSH_rollover_bits & 0b1110) == 0b1110) {
    game_add_score(5000);
    bonus_multiplier++;
    drinks += 5;
    CSH_rollover_bits = 0;
  }
}

static void shift_CSH_rollover(boolean right) {
  if (right) {
    CSH_rollover_bits >>= 1;
  } else {
    CSH_rollover_bits <<= 1;
  }

  if (CSH_rollover_bits & 0b10000) {
    CSH_rollover_bits ^= 0b10010;
  }
  
  if (CSH_rollover_bits & 0b00001) {
    CSH_rollover_bits ^= 0b01001;
  }
}

// roll = 0 is right
static void rollover_1976(byte roll) {
  loggf(TRACE, "1-9-7-6 Rollover %u", roll);
  
  drinks++;

  rollover_1976_bits |= 1 << (roll + 1);
  
  if (roll == 0 || roll == 3) {
    game_add_score(10000);
  } else {
    game_add_score(2000);
  }

  if ((rollover_1976_bits & 0b011110) == 0b011110) {
    bonus_multiplier++;
    drinks += 5;
    rollover_1976_bits = 0;
  }
}

static void shift_1976_rollover(boolean right) {
  if (right) {
    rollover_1976_bits >>= 1;
  } else {
    rollover_1976_bits <<= 1;
  }

  if (rollover_1976_bits & 0b100000) {
    rollover_1976_bits ^= 0b100010;
  }
  
  if (rollover_1976_bits & 0b000001) {
    rollover_1976_bits ^= 0b010001;
  }
}

static void vader_lobby(boolean left) {
  loggf(TRACE, "Vader Lobby (%s)", left ? "LEFT" : "RIGHT");

  // im samrt i know conts * vs * const :)
  byte * const hit_count = left ? &vader_lobby_hit_count_left : &vader_lobby_hit_count_right;

  (*hit_count)++;
  if (*hit_count >= 5) {
    *hit_count = 0;
    devcade_games++;
  }
  
  game_add_score(15000);
  drinks += 5;
}

static void lounge_rollover_behind() {
  logg(TRACE, "Lounge Rollover Behind");
  
  game_add_score(20000);
}

static void drop_targets(byte num, boolean rtp) {
  loggf(TRACE, "Drop Targets (%s) %u", rtp ? "RTP" : "Server Room", num);

  byte * const order = rtp ? &rtp_order : &server_room_order;

  if (current_multiball == PCP) {
    if (PCP_phase == 0) {
      game_add_score(20000);
      const boolean all =
	SENSOR_dt11
	&& SENSOR_dt12
	&& SENSOR_dt13
	&& SENSOR_dt21
	&& SENSOR_dt22
	&& SENSOR_dt23;
      
      if (all) {
	PCP_phase = 1;
      }
    }
    return;
  }
  
  game_add_score(7000);

  if (num == *order && current_mode == MODE_NONE) {
    (*order)++;
  }
  if (*order == 3) {
    *order = 0;
    game_add_score(50000);

    if (rtp) {
      devcade_games++;
    } else if (current_mode == MODE_NONE) {
      available_modes |= OPCOMMATHON;
    }
  }
}

static void drop_target_reset() {
  static unsigned long drop1_timeout = 0;
  static unsigned long drop2_timeout = 0;
  if (current_multiball == PCP && PCP_phase == 0) {
    return;
  }
  
  if (SENSOR_dt11 && SENSOR_dt12 && SENSOR_dt13 && millis() > drop1_timeout) {
    drop1_timeout = millis() + 250;
    activate_output(OUTPUT_drop1);
  }
  
  if (SENSOR_dt21 && SENSOR_dt22 && SENSOR_dt23 && millis() > drop2_timeout) {
    drop2_timeout = millis() + 250;
    activate_output(OUTPUT_drop2);
  }
}

// Star Rollover
static void party_button_rollover_behind() {
  logg(TRACE, "Party Button Rollover Behind");
  
  game_add_score(10000);

  if (current_mode != MODE_NONE) {
    mystery_count++;
    if (mystery_count >= 2 && current_mode == MODE_NONE && current_multiball == MULTI_NONE) {
      mystery_available = true;
      mystery_count = 0;
    }
  }
}

static void party_button() {
  logg(TRACE, "Party Button");

  if (current_multiball == PARTY_BUTTON) {
    if (party_button_multi_hit_count < 5) {
      party_button_multi_hit_count++;
      game_add_score(100000);
    } else {
      game_add_score(200000);
    }

    return;
  }
  
  game_add_score(30000);

  party_button_hit_count++;

  if (party_button_hit_count == 2 && current_mode == MODE_NONE) {
    current_mode = YOU_DID_WHAT;
  }

  if (party_button_hit_count == 6) {
    activate_multiball(PARTY_BUTTON);
  }

  if (party_button_hit_count == 8) {
    extra_ball_available = true;
    party_button_hit_count = 0;
  }
}

static void lounge_rectangle_target() {
  logg(TRACE, "Lounge Rectangle Target");
  
  const unsigned long time = millis();
  if (time - mode_finish_timer <= 10000) {
    const double ratio = (10000 - time + mode_finish_timer) / 10000.0;
    game_add_score(ratio * 1000 * (drinks + devcade_games + imagine_projects));
  }
  
  game_add_score(1000);
  lounge_rect_counter++;
  if (lounge_rect_counter >= 5) {
    lounge_rect_counter = 0;
    imagine_projects++;
  }
}

static void elevator_button() {
  logg(TRACE, "Elevator Button");

  game_add_score(12000);

  if (current_multiball != MULTI_NONE && current_mode != MODE_NONE) {
    elevator_count++;
  }

  if (elevator_count >= 3) {
    available_modes |= HOUSE_MEETING;
  }
}

static void do_mystery() {
  const unsigned long rand = random(100);
  
  if (rand <= 20) {
    if (game_won) {
      game_add_score(50000);
    } else {
      game_add_score(500000);
    }
  } else if (rand <= 40) {
    available_multiballs |= BAGELS;
  } else if (rand <= 60) {
    extra_ball_available = true;
  } else if (rand <= 80) {
    drinks += 5;
  } else {
    devcade_games += 2;
  }
}

static void scoop() {
  game_add_score(2000);

  if (mystery_available) {
    mystery_available = false;
    do_mystery();
  }

  if (extra_ball_available) {
    set_extra_ball();
  }

  if (available_modes > 0 && current_mode == MODE_NONE) {
    current_mode = (Mode)(1 << random(3));
  }

  if (!kick_scoop) {  
    kick_scoop = true;
    kick_scoop_timer = millis() + 1000;
  }
}

static void BIG_targets(byte which) {
  static byte big_target_bits = 0b000;
  static byte big_target_order = 0;
  static byte big_target_hits = 0;
  
  game_add_score(200);
  drinks++;

  if (current_mode == MODE_NONE) {
    //big_target
  }
}

void garret_game_loop() {
  // Pop bumpers
  if (LATCH_pop1) {
    pop_bumper();
  }
  if (LATCH_pop2) {
    pop_bumper();
  }
  if (LATCH_pop3) {
    pop_bumper();
  }

  // Slingshots
  if (LATCH_sling1) {
    slingshot();
  }
  if (LATCH_sling2) {
    slingshot();
  }

  // C-S-H Rollovers
  if (LATCH_roll3) {
    CSH_rollover(2);
  }
  if (LATCH_roll4) {
    CSH_rollover(1);
  }
  if (LATCH_roll5) {
    CSH_rollover(0);
  }

  // 1-9-7-6 Rollovers
  if (LATCH_roll15) {
    rollover_1976(3);
  }
  if (LATCH_roll14) {
    rollover_1976(2);
  }
  if (LATCH_roll13) {
    rollover_1976(1);
  }
  if (LATCH_roll12) {
    rollover_1976(0);
  }

  // Rollover shifters
  if (LATCH_fl1) {
    shift_1976_rollover(false);
    shift_CSH_rollover(false);
  }
  if (LATCH_fl2) {
    shift_1976_rollover(true);
    shift_CSH_rollover(true);
  }

  // Vader Lobby rollovers
  if (LATCH_roll1) {
    vader_lobby(true);
  }
  if (LATCH_roll2) {
    vader_lobby(false);
  }

  // Lounge Rollover Behind
  if (LATCH_roll9) {
    lounge_rollover_behind();
  }

  // R-T-P drop targets
  if (LATCH_dt11) {
    drop_targets(0, true);
  }
  if (LATCH_dt12) {
    drop_targets(1, true);
  }
  if (LATCH_dt13) {
    drop_targets(2, true);
  }
  
  // Server Room drop targets
  if (LATCH_dt21) {
    drop_targets(0, false);
  }
  if (LATCH_dt22) {
    drop_targets(1, false);
  }
  if (LATCH_dt23) {
    drop_targets(2, false);
  }

  // Star Rollover (Party Button Rollover Behind)
  if (LATCH_star_roll) {
    party_button_rollover_behind();
  }

  if (LATCH_rr1) {
    party_button();
  }
  if (LATCH_rr2) {
    elevator_button();
  }

  if (LATCH_large_rect) {
    lounge_rectangle_target();
  }

  if (LATCH_scoop) {
    scoop();
  }
  if (kick_scoop && millis() > kick_scoop_timer) {
    kick_scoop = false;
    activate_output(OUTPUT_scoop);
  }

  drop_target_reset();

  logg(TRACE, "Garret Game Loop");
}

void garret_game_end_ball() {
  logg(TRACE, "Garret Game End Ball");
  game_add_score(bonus_multiplier * (5000 * drinks + 50000 * devcade_games + 100000 * imagine_projects));
}
