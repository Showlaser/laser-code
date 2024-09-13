#include "AudienceShutter.h"
#include "Arduino.h"

/**
 @brief Moves the shutter up
*/
void AudienceShutter::moveShutterUp() {
  analogWrite(28, 125);
  analogWrite(29, 0);
}

/**
 @brief Moves the shutter down
*/
void AudienceShutter::moveShutterDown() {
  analogWrite(28, 0);
  analogWrite(29, 125);
}

void AudienceShutter::stopMoving() {
  analogWrite(28, 0);
  analogWrite(29, 0);
}