#include "NetworkPlayMode.h"
#include <utility>

NetworkPlayMode::NetworkPlayMode(Laser &laser, RealtimePlayer &player)
    : ShowPlayerMode(laser, player)
{
  _loop = true; // a live pattern/animation loops until replaced or stopped
}

LaserMode NetworkPlayMode::getModeName()
{
  return LaserMode::Network;
}

void NetworkPlayMode::preExecute()
{
  // A new blob arrived while we are already playing one: tear down so execute()
  // re-acquires the new blob (atomic swap at a frame boundary).
  if (LiveShowPending && _mem.isOpen())
  {
    restart();
  }
}

ShowSource *NetworkPlayMode::acquireSource()
{
  if (!LiveShowPending)
  {
    return nullptr; // nothing new to play; idle
  }
  LiveShowPending = false;

  if (!_mem.load(std::move(LiveShowData)))
  {
    Serial.println("Live show: invalid .lzs blob");
    CurrentLaserMode = LaserMode::NotSelected; // nothing to play; leave the mode
    return nullptr;
  }

  return &_mem;
}

void NetworkPlayMode::releaseSource()
{
  _mem.close();
}

void NetworkPlayMode::onProducingFinished()
{
  // Only reached if the uploaded blob had no drawable frames; go idle.
  CurrentLaserMode = LaserMode::NotSelected;
}
