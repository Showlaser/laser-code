#ifndef LASER_H
#define LASER_H

#include "Arduino.h"
#include <NativeEthernet.h>

class Laser {
  public:
    Laser();
    bool hardwareSelfCheck();

  private:
};

#endif