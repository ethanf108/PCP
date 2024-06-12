#include <Arduino.h>

#ifndef __PCP_LOG_H
#define __PCP_LOG_H

#define LOG_BAUD_RATE 115200
#define LOG_CRLF false // ? "\r\n" : "\n"
#define LOG_BUFFER_SIZE 200
#define LOG_COLORS true

typedef enum: byte {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5,
} LogLevel;

const char * const LEVEL_STRINGS[8] = {"TRACE", "DEBUG", " INFO", " WARN", "ERROR", "FATAL"};
const char * const LEVEL_COLOR_STRINGS[8] = {
  "\x1b[36mTRACE\x1b[0m",
  "DEBUG",
  "\x1b[32m INFO\x1b[0m",
  "\x1b[33m WARN\x1b[0m",
  "\x1b[31mERROR\x1b[0m",
  "\x1b[37;41mFATAL\x1b[0m"
};

void log_setup(void);
void set_log_level(LogLevel);
LogLevel get_log_level(void);

// Log a string that's in PROGMEM (Save memory)
void logg_(LogLevel, const __FlashStringHelper*);
#define logg(ll, s) logg_(ll, F(s))

// Same but for ptrinf, the format string is in PROGMEM (I think)
void logf_(LogLevel, const char*, ...);
#define loggf(ll, s, ...) logf_(ll, PSTR(s), __VA_ARGS__)

// For a string not in PROGMEM. Try not to use this
// And be safe when using it. Too many could cause mysterious
// crashes due to running out of memory.
void loggv_(LogLevel, const __FlashStringHelper*, char*);
#define loggv3(ll, before, s) loggv_(ll, F(before), s)
#define loggv2(ll, s) loggv_(ll, NULL, s)
#define GET_MACRO(_1,_2,_3,NAME,...) NAME
#define loggv(...) GET_MACRO(__VA_ARGS__, loggv3, loggv2)(__VA_ARGS__)
  

#endif
