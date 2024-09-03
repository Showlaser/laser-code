#include "AudienceShutter.h"
#include "Arduino.h"

/**
 @brief Moves the shutter up
*/
void AudienceShutter::moveShutterUp() {
  Serial.println("Move up");
  analogWrite(28, 125);
  analogWrite(29, 0);
}

/**
 @brief Moves the shutter down
*/
void AudienceShutter::moveShutterDown() {
  Serial.println("Move down");
  analogWrite(28, 0);
  analogWrite(29, 125);
}

void AudienceShutter::stopMoving() {
    Serial.println("Stop");
  analogWrite(28, 0);
  analogWrite(29, 0);
}