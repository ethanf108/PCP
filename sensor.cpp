#include "sensor.h"
#include "log.h"
#include <Arduino.h>

static unsigned long long sensors = 0;
static unsigned long long sensors_last = 0;
static unsigned long long sensor_latches = 0;
static unsigned long debounce[64] = {0};

void sensor_setup() {
  for (byte pin = min(SENSOR_ROW_OUT_START, SENSOR_COL_IN_START); pin <= 45; pin++) {
    pinMode(pin, pin % 2 == SENSOR_ROW_OUT_START % 2 ? OUTPUT : INPUT);
  }
  logg(TRACE, "Sensor setup complete");
}

// Timing suggests this never takes more than 2ms (pre debounce)
void sensor_refresh() {
  sensors = 0;
  const unsigned long current_millis = millis(); // Saves computation time
  for (byte row = 0; row < 8; row++) {
    for (byte i = 0; i < 8; i++) {
      digitalWrite(SENSOR_ROW_OUT_START + i * 2, row == i ? HIGH : LOW);
    }
    
    for (byte col = 0; col < 7; col++) {
      const boolean status = digitalRead(SENSOR_COL_IN_START + col * 2);
      const byte index = row * 7 + col;
      if (status != (boolean)(sensors_last & (1ULL << index))) {
	debounce[index] = millis();
	sensors_last ^= (1ULL << index);
      } else if (current_millis - debounce[index] > DEBOUNCE_TIME && status) {
	sensors |= 1ULL << index;
      }
    }
  }
  logg(TRACE, "Sensor refresh complete");
}

boolean sensor_read(byte sensor) {
  return sensors & (1ULL << sensor);
}

static void sensor_update_latch(byte sensor, boolean inverse) {
  if (sensor_read(sensor) ^ inverse) {
    sensor_latches |= 1ULL << sensor;
  } else {
    sensor_latches &= ~(1ULL << sensor);
  }
}

boolean sensor_read_latch(byte sensor) {
  const boolean ret = sensor_read(sensor)
    && !(sensor_latches & (1ULL << sensor));

  sensor_update_latch(sensor, false);

  return ret;
}

boolean sensor_read_latch_inverse(byte sensor) {
  const boolean ret = !sensor_read(sensor)
    && !(sensor_latches & (1ULL << sensor));

  sensor_update_latch(sensor, true);

  return ret;
}

byte trough_count() {
  byte troughs = 0;
  
  while (sensor_read(27 - troughs)) {
    troughs++;
  }

  if (!SENSOR_trough_ready) {
    return 0;
  }
  
  return troughs + 1;
}
