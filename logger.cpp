// Logger for the ESP32, the raspberry pi will be using a different logger implementation

#include "logger.h"
#include "Arduino.h"

void start_logger (const char* type) {
    printf("[%d][%s] ", time(), type);
}
void end_logger () {
    printf("\n");
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
