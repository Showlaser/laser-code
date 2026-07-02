#include "PlaySDFileMode.h"

PlaySDFileMode::PlaySDFileMode(Laser &laser, RealtimePlayer &player)
    : ShowPlayerMode(laser, player)
{
  _loop = false; // an SD lasershow plays once, then the mode ends
}

LaserMode PlaySDFileMode::getModeName()
{
  return LaserMode::SDCardMode;
}

ShowSource *PlaySDFileMode::acquireSource()
{
  if (SelectedSDCardFilename == "")
  {
    return nullptr;
  }

  if (!_file.open(SelectedSDCardFilename))
  {
    Serial.println("Failed to open show file: " + SelectedSDCardFilename);
    CurrentLaserMode = LaserMode::NotSelected;
    return nullptr;
  }

  return &_file;
}

void PlaySDFileMode::releaseSource()
{
  _file.close();
}

void PlaySDFileMode::onProducingFinished()
{
  CurrentLaserMode = LaserMode::NotSelected;
}
