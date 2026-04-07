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
};
#endif // LOGGER_H