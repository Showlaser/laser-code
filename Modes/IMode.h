#ifndef IMODE_H
#define IMODE_H

#include "../GlobalConfig.h"
#include "../Settings.h"

class IMode {
public:
  /**
  @brief Executes the mode
  */
  virtual void execute();
  virtual void stop();
  virtual LaserMode getModeName();
  virtual ~IMode() {}

private:
};

#endif