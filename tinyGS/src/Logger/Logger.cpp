/*
  Logger.cpp - Log class
  
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

#include "Logger.h"
#include "time.h"

char Log::logIdx = 1;
Log::LoggingLevels Log::logLevel = LOG_LEVEL;
char Log::log[MAX_LOG_SIZE] = "";
QueueHandle_t Log::logQueue = nullptr;
TaskHandle_t Log::logTask = nullptr;
bool Log::asyncEnabled = false;

void Log::initAsync()
{
  if (asyncEnabled) return;
  
  logQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogMessage));
  if (logQueue == nullptr) {
    // Cannot use Log::console here as we're initializing the async system
    // Fall back to direct serial for critical init errors
    return;
  }
  
  BaseType_t result = xTaskCreate(
    logTaskFunction,
    "LogTask",
    4096,  // Stack size
    nullptr,
    1,     // Low priority (tskIDLE_PRIORITY + 1)
    &logTask
  );
  
  if (result != pdPASS) {
    // Cannot use Log::console here as we're initializing the async system
    vQueueDelete(logQueue);
    logQueue = nullptr;
    return;
  }
  
  asyncEnabled = true;
  // Successfully initialized async logging
}

void Log::logTaskFunction(void* parameter)
{
  LogMessage msg;
  while (true) {
    if (xQueueReceive(logQueue, &msg, portMAX_DELAY) == pdTRUE) {
      // Process the log message (can block here)
      AddLog(msg.level, msg.message);
    }
  }
}

void Log::consoleAsync(const char* formatP, ...)
{
  if (!asyncEnabled) {
    // Fallback to synchronous logging
    va_list arg;
    char buffer[256];
    va_start(arg, formatP);
    vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
    va_end(arg);
    AddLog(LOG_LEVEL_NONE, buffer);
    return;
  }
  
  LogMessage msg;
  va_list arg;
  va_start(arg, formatP);
  vsnprintf_P(msg.message, sizeof(msg.message), formatP, arg);
  va_end(arg);
  msg.level = LOG_LEVEL_NONE;
  
  // Non-blocking send - if queue is full, drop the message
  if (xQueueSend(logQueue, &msg, 0) != pdTRUE) {
    // Queue full - message dropped (silent failure to avoid recursion)
  }
}

void Log::errorAsync(const char* formatP, ...)
{
  if (!asyncEnabled) {
    // Fallback to synchronous logging
    va_list arg;
    char buffer[256];
    va_start(arg, formatP);
    vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
    va_end(arg);
    AddLog(LOG_LEVEL_ERROR, buffer);
    return;
  }
  
  LogMessage msg;
  va_list arg;
  va_start(arg, formatP);
  vsnprintf_P(msg.message, sizeof(msg.message), formatP, arg);
  va_end(arg);
  msg.level = LOG_LEVEL_ERROR;
  
  // Non-blocking send - if queue is full, drop the message
  if (xQueueSend(logQueue, &msg, 0) != pdTRUE) {
    // Queue full - message dropped
  }
}

void Log::debugAsync(const char* formatP, ...)
{
  if (!asyncEnabled) {
    // Fallback to synchronous logging
    va_list arg;
    char buffer[321];
    va_start(arg, formatP);
    vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
    va_end(arg);
    AddLog(LOG_LEVEL_DEBUG, buffer);
    return;
  }
  
  LogMessage msg;
  va_list arg;
  va_start(arg, formatP);
  vsnprintf_P(msg.message, sizeof(msg.message), formatP, arg);
  va_end(arg);
  msg.level = LOG_LEVEL_DEBUG;
  
  // Non-blocking send - if queue is full, drop the message
  if (xQueueSend(logQueue, &msg, 0) != pdTRUE) {
    // Queue full - message dropped
  }
}

void Log::console(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_NONE, buffer);
}

void Log::error(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_ERROR, buffer);
}

void Log::info(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_INFO, buffer);
}

void Log::debug(const char* formatP, ...)
{
  va_list arg;
  char buffer[321];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_DEBUG, buffer);
}

// Based on arendst/Tasmota addLog (support.ino)
void Log::AddLog(Log::LoggingLevels level, const char* logData)
{
  if (level > Log::logLevel)
    return;

  char timeStr[10];  // "13:45:21 "
  time_t currentTime = time (NULL);
  if (currentTime > 0) {
      struct tm *timeinfo = localtime (&currentTime);
      snprintf_P (timeStr, sizeof (timeStr), "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  }
  else {
      timeStr[0] = '\0';
  }
  
  Serial.printf (PSTR ("%s%s\n"), timeStr, logData);

  // Delimited, zero-terminated buffer of log lines.
  // Each entry has this format: [index][log data]['\1']
  while (logIdx == log[0] ||  // If log already holds the next index, remove it
          strlen(log) + strlen(logData) + 13 > MAX_LOG_SIZE)  // 13 = idx + time + '\1' + '\0'
  {
    char* it = log;
    it++;                                // Skip web_log_index
    it += strchrspn(it, '\1');           // Skip log line
    it++;                                // Skip delimiting "\1"
    memmove(log, it, MAX_LOG_SIZE -(it-log));  // Move buffer forward to remove oldest log line
  }
  
  snprintf_P(log, sizeof(log), PSTR("%s%c%s%s\1"), log, logIdx++, timeStr, logData);

  logIdx &= 0xFF;
  if (!logIdx) 
    logIdx++;       // Index 0 is not allowed as it is the end of char string*/
}

void Log::getLog(uint32_t idx, char** entry_pp, size_t* len_p)
{
  char* entry_p = nullptr;
  size_t len = 0;

  if (idx) {
    char* it = log;
    do {
      uint32_t cur_idx = *it;
      it++;
      size_t tmp = strchrspn(it, '\1');
      tmp++;                             // Skip terminating '\1'
      if (cur_idx == idx) {              // Found the requested entry
        len = tmp;
        entry_p = it;
        break;
      }
      it += tmp;
    } while (it < log + MAX_LOG_SIZE && *it != '\0');
  }
  *entry_pp = entry_p;
  *len_p = len;
}

// Get span until single character in string
size_t Log::strchrspn(const char *str1, int character)
{
  size_t ret = 0;
  char *start = (char*)str1;
  char *end = strchr(str1, character);
  if (end) ret = end - start;
  return ret;
}

char Log::getLogIdx()
{
  return logIdx;
}

void Log::setLogLevel(LoggingLevels level)
{
  logLevel = level;
}

void Log::log_packet(uint8_t *packet, size_t size){
  //Print out Packet Data in Hex/ASCII form
  const int bytes_per_line=16;
  const int longLinea=bytes_per_line*3;//Add the space after each byte
  char cadena[longLinea+1];
  char ascii[bytes_per_line+1];
  int j=0; int k=0;
  Log::consoleAsync(PSTR("-----------------------------------------------------------------"));
  for (int i=0;i<size;i++){
      j=3*(i%bytes_per_line); //Index for the Hex Data
      k=(i%bytes_per_line); //Index for the ASCII Data
      //Preparation of the Hex data line
      sprintf(cadena + j, "%02X ", packet[i]);
      //Preparation of the ASCII data line
      if (packet[i]>=32 && packet[i]<127){
        sprintf(ascii + k, "%c", packet[i]);
      }else{
        sprintf(ascii + k, "%c", 0x2E);
      }
      //Check for end of line or end of packet
      if ((k==bytes_per_line-1) || i==size-1) {
        //If end of packet fill in with spaces
        if (i==size-1){
          int m=0;
          //Add spaces until complete Hex Data Linea
          for (m=j+3;m<(longLinea);m+=3){
            sprintf(cadena + m, "   ");
          }
          int n=0;
          //Add spaces until complete ASCII Line
          for (n=k+1;n<(bytes_per_line);n++){
            sprintf(ascii + n, " ");
          }
        }
        cadena[longLinea]=0;
        ascii[bytes_per_line]=0;
        //Final Data/ASCII Line print on screen
        Log::consoleAsync(PSTR("%s %s"),cadena,ascii);
        cadena[0]=0;
        ascii[0]=0;
        }
  }
  Log::consoleAsync(PSTR("-----------------------------------------------------------------"));
}

void Log::log_packet_hex(uint8_t *packet, size_t size){
  //Print out Packet Data in Hex form
  const int bytes_per_line=32;
  const int longLinea=bytes_per_line*2;
  char cadena[longLinea+1];
  int j=0; int k=0;
  //Log::console(PSTR("Logging packet..."));
  for (int i=0;i<size;i++){
      j=2*(i%bytes_per_line); //Index for the Hex Data
      k=(i%bytes_per_line); //Index for the written byte in line
      //Preparation of the Hex data line
      sprintf(cadena + j, "%02X", packet[i]);
      //Check for end of line or end of packet
      if ((k==bytes_per_line-1) || i==size-1) {
        //If end of packet fill in with spaces
        if (i==size-1){
          int m=0;
          //Add spaces until complete Hex Data Linea
          for (m=j+2;m<(longLinea);m+=2){
            sprintf(cadena + m, "  ");
          }
          int n=0;
        }
        cadena[longLinea]=0;
        //Final Data Line print on screen
        Log::consoleAsync(PSTR("%s"),cadena);
        cadena[0]=0;
        }
  }
  Log::consoleAsync(PSTR(" "));
}

void Log::log_packet_ax25(uint8_t *packet, size_t size){
  //Print out the ax25 header in first place and the payload as Hex/ASCII form
  const int bytes_per_line=16;
  const int longLinea=bytes_per_line*3;//Add the space after each byte
  char cadena[longLinea+1];
  char ascii[bytes_per_line+1];
  int j=0; int k=0;
 
  // Usar buffer en stack con tamaño máximo razonable
  // Si size es muy grande, limitamos para evitar stack overflow
  const size_t max_packet_size = 512;
  uint8_t packet_aux_buffer[max_packet_size];
  uint8_t *packet_aux = packet_aux_buffer;
  size_t effective_size = (size > max_packet_size) ? max_packet_size : size;

  if (size>=16){
  //////////////////////////////////////////////////////////////////////
  // AX.25 HEADER
  //////////////////////////////////////////////////////////////////////
  //--------------------------- AX.25 HEADER ---------------------------
  //XX XX XX XX XX XX XX | Destination Address.....: DDDDDD     SSID: XX
  //XX XX XX XX XX XX XX | Source Address..........: SSSSSS     SSID: XX
  //XX                   | Control.................: DEC
  //XX                   | PID.....................: DEC
  //--------------------------------------------------------------------
  /////////////////////////////
  //6 bytes Destination Address
  /////////////////////////////
  uint8_t byte_aux=0;
  for (int i=0;i<6;i++){
    /*
    For the Destination Address and Source Address before getting the ASCII value
    of each byte, the byte should be shifted one bit to the RIGHT, in this way, 
    a bit zero is inserted by the LEFT. 
    */
    byte_aux=packet[i]>>1;
    if (byte_aux>=32 and byte_aux<=127){
      sprintf(ascii + i, "%c", byte_aux);
    }else{
      sprintf(ascii + i, "%c", 32);
    }
  }
  Log::consoleAsync(PSTR("------------------------- AX.25 HEADER --------------------------"));
  Log::consoleAsync(PSTR("%02X %02X %02X %02X %02X %02X %02X | Destination Address.....: %s  SSID: %i")
                     ,packet[0],packet[1],packet[2],packet[3],packet[4],packet[5],packet[6]
                     ,ascii,(packet[6]&(0x1E))>>1);
  ascii[0]=0;
  /////////////////////////////
  //6 bytes Source Address
  /////////////////////////////
  for (int i=7;i<13;i++){
    byte_aux=packet[i]>>1;
    if (byte_aux>=32 and byte_aux<=127){
     sprintf(ascii + i-7, "%c", byte_aux);
    }else{
     sprintf(ascii + i-7, "%c", 32);
    }
  }
  Log::consoleAsync(PSTR("%02X %02X %02X %02X %02X %02X %02X | Source Address..........: %s  SSID: %i")
                     ,packet[7],packet[8],packet[9],packet[10],packet[11],packet[12],packet[13]
                     ,ascii,(packet[13]&(0x1E))>>1);
  ascii[0]=0;
  //1 byte  Control
  Log::consoleAsync(PSTR("%02X                   | Control.................: %03i"),packet[14],packet[14]);
  //1 byte  PID
  Log::consoleAsync(PSTR("%02X                   | PID.....................: %03i"),packet[15],packet[15]);

  ////////////////////////////////////////////////////////////////////////////////////////
  //AX.25 PAYLOAD
  ////////////////////////////////////////////////////////////////////////////////////////
  int counter=0;
  size_t copy_size = (size > effective_size) ? effective_size : size;
  for (int i=16;i<copy_size;i++){
    packet_aux[counter]=packet[i];
    counter++;
    }
  Log::log_packet(packet_aux, (copy_size > 16) ? copy_size-16 : 0);
  
  }else{
    Log::consoleAsync(PSTR(" *** Length less than 16 bytes. Packet not printed ***"));
  }
}