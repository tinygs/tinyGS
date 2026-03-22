/*
  Logger.h - Log class
  
  Copyright (C) 2020 -2021 @G4lile0, @gmag12 and @dev_4m1g0

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LOGGER_H
#define LOGGER_H

#include "Arduino.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define MAX_LOG_SIZE 4000
#define LOG_LEVEL    LOG_LEVEL_NONE
#define MAX_ASYNC_LOG_SIZE 320
#define LOG_QUEUE_SIZE 30

class Log {
public:
  enum LoggingLevels {LOG_LEVEL_NONE, LOG_LEVEL_ERROR, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG};
  
  // Asynchronous logging functions (non-blocking)
  static void initAsync();
  static void consoleAsync(const char* logData, ...);
  static void errorAsync(const char* logData, ...);
  static void debugAsync(const char* logData, ...);
  
  // Synchronous logging functions (blocking - legacy)
  static void console(const char* logData, ...);
  static void error(const char* logData, ...);
  static void info(const char* logData, ...);
  static void debug(const char* logData, ...);
  static void getLog(uint32_t idx, char** entry_pp, size_t* len_p);
  static char getLogIdx();
  static void setLogLevel(LoggingLevels level);
  static void log_packet(uint8_t *packet, size_t size);
  static void log_packet_hex(uint8_t *packet, size_t size);
  static void log_packet_ax25(uint8_t *packet, size_t size);
  
private:
  struct LogMessage {
    char message[MAX_ASYNC_LOG_SIZE];
    LoggingLevels level;
  };
  
  static void AddLog(LoggingLevels logLevel, const char* logData);
  static void AddLogAsync(LoggingLevels logLevel, const char* logData);
  static void logTaskFunction(void* parameter);
  static size_t strchrspn(const char *str1, int character);
  static char log[MAX_LOG_SIZE];
  static char logIdx;
  static LoggingLevels logLevel;
  static QueueHandle_t logQueue;
  static TaskHandle_t logTask;
  static bool asyncEnabled;
  static SemaphoreHandle_t logMutex;
};

// ---------------------------------------------------------------------------
// Compile-time log-level macros
// ---------------------------------------------------------------------------
// LOG_BUILD_LEVEL controls which log calls survive compilation.
//   0 = ALL   (debug + info + error + console)
//   1 = INFO  (info + error + console)          — default for production
//   2 = ERROR (error + console only)
//   3 = NONE  (console only — always present)
//
// Set via build flag: -DLOG_BUILD_LEVEL=0  (verbose dev builds)
// Console/consoleAsync are ALWAYS compiled in.
// ---------------------------------------------------------------------------
#ifndef LOG_BUILD_LEVEL
  #define LOG_BUILD_LEVEL 1
#endif

// --- Always available ---
#define LOG_CONSOLE(...)        Log::console(__VA_ARGS__)
#define LOG_CONSOLE_ASYNC(...)  Log::consoleAsync(__VA_ARGS__)

// --- error / errorAsync: compiled when level <= 2 ---
#if LOG_BUILD_LEVEL <= 2
  #define LOG_ERROR(...)        Log::error(__VA_ARGS__)
  #define LOG_ERROR_ASYNC(...)  Log::errorAsync(__VA_ARGS__)
#else
  #define LOG_ERROR(...)        ((void)0)
  #define LOG_ERROR_ASYNC(...)  ((void)0)
#endif

// --- info: compiled when level <= 1 ---
#if LOG_BUILD_LEVEL <= 1
  #define LOG_INFO(...)         Log::info(__VA_ARGS__)
#else
  #define LOG_INFO(...)         ((void)0)
#endif

// --- debug / debugAsync: compiled when level == 0 ---
#if LOG_BUILD_LEVEL <= 0
  #define LOG_DEBUG(...)        Log::debug(__VA_ARGS__)
  #define LOG_DEBUG_ASYNC(...)  Log::debugAsync(__VA_ARGS__)
#else
  #define LOG_DEBUG(...)        ((void)0)
  #define LOG_DEBUG_ASYNC(...)  ((void)0)
#endif

#endif // LOGGER_H