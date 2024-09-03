#ifndef IMODE_H
#define IMODE_H

#include "GlobalConfig.h"

class IMode {
public:
  /**
  @brief Executes the mode
  */
  virtual void execute();
  virtual LaserMode getModeName();
  virtual ~IMode() {}

private:
};

#endif