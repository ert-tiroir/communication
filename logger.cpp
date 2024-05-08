// Logger for the ESP32, the raspberry pi will be using a different logger implementation

#include "logger.h"
#include "Arduino.h"

int logger_locked = false;
void lock () {
    while (logger_locked) delayMicroseconds(1);
    logger_locked = true;
}

void start_logger (const char* type) {
    lock();
    printf("[%ld][%s] ", millis(), type);
}
void end_logger () {
    printf("\n");
    logger_locked = false;
}
void slogger (const char* message) {
    printf("%s", message);
}
void ilogger (int message) {
    printf("%d", message);
}
void llogger (long long message) {
    printf("%lld", message);
}
