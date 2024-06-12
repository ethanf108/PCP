#include <Arduino.h>
#include "log.h"
#include "sensor.h"
#include "output.h"
#include "display.h"
#include "game.h"
#include "garret_game.h"

static game_def game = {
  .setup = &garret_game_setup,
  .start_ball = &garret_game_start_ball,
  .loop = &garret_game_loop,
  .end_ball = &garret_game_end_ball
};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  log_setup();  
  
  set_log_level(DEBUG);
  //display_setup();
  
  sensor_setup();
  output_setup();
  
  game_setup(&game);
  
  logg(INFO, "All Setup complete");
}

void loop() {
  sensor_refresh();
  game_loop();

  if (false) {
    logg(WARN, "SCOOP");
  }
  
  logg(TRACE, "Board Loop complete");
}
