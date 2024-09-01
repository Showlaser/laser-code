#include "AudienceShutter.h"
#include "Arduino.h"

/**
 @brief Moves the shutter up
*/
void AudienceShutter::moveShutterUp() {
  digitalWrite(20, HIGH);
  digitalWrite(21, LOW);
}

/**
 @brief Moves the shutter down
*/
void AudienceShutter::moveShutterDown() {
  digitalWrite(20, LOW);
  digitalWrite(21, HIGH);
}

void AudienceShutter::stopMoving() {
  digitalWrite(20, LOW);
  digitalWrite(21, LOW);
}