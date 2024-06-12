#include "log.h"
#include <Arduino.h>


static LogLevel current_level = WARN;
static char log_buffer[LOG_BUFFER_SIZE] = {0};

void log_setup() {
  Serial.begin(LOG_BAUD_RATE);
}

void set_log_level(LogLevel level) {
  current_level = level;
}

LogLevel get_log_level() {
  return current_level;
}

static void print_header(LogLevel level) {
  Serial.print(F("["));
  if (LOG_COLORS) {
    //strncpy_P(log_buffer, (char *)pgm_read_ptr(&(LEVEL_COLOR_STRINGS[level])), LOG_BUFFER_SIZE);
    Serial.print(LEVEL_COLOR_STRINGS[level]);
  } else {
    Serial.print(LEVEL_STRINGS[level]);
  }
  Serial.print("] ");
}

void logg_(LogLevel level, const __FlashStringHelper* str) {
  if (level < current_level) {
    return;
  }

  print_header(level);
  Serial.print(str);
  
  if (LOG_CRLF) {
    Serial.print("\r\n");
  } else {
    Serial.print("\n");
  }
}

void logf_(LogLevel level, const char* format, ...) {
  if (level < current_level) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  
  vsnprintf_P(log_buffer, LOG_BUFFER_SIZE, format, args);

  print_header(level);
  Serial.print(log_buffer);
  
  if (LOG_CRLF) {
    Serial.print("\r\n");
  } else {
    Serial.print("\n");
  }
}

void loggv_(LogLevel level, const __FlashStringHelper* before, char* str) {
  logg(TRACE, "loggv used. Be careful"); // LOL

  if (level < current_level) {
    return;
  }

  print_header(level);
  if (before != NULL) {
    Serial.print(before);
  }
  Serial.print(str);
  
  if (LOG_CRLF) {
    Serial.print("\r\n");
  } else {
    Serial.print("\n");
  }
}
