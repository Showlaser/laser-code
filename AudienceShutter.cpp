#include "AudienceShutter.h"
#include "Arduino.h"

/**
 @brief Moves the shutter up
*/
void AudienceShutter::moveShutterUp() {
  digitalWrite(20, HIGH);
}

/**
 @brief Moves the shutter down
*/
void AudienceShutter::moveShutterDown() {
  digitalWrite(21, HIGH);
}