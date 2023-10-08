#include "AudienceShutter.h"

/**
 @brief Moves the shutter up
*/
void AudienceShutter::moveShutterUp() {
  digitalWrite(20, High);
}

/**
 @brief Moves the shutter down
*/
void AudienceShutter::moveShutterDown() {
  digitalWrite(21, High);
}