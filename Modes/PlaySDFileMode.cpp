#include "PlaySDFileMode.h"
PlaySDFileMode::PlaySDFileMode(Laser &laser) : _laser(laser)
{
}

unsigned long PlaySDFileMode::roundDown(unsigned long numToRound)
{
    int multiple = 10;
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    return numToRound - remainder;
}

JsonObject PlaySDFileMode::getCurrentClusterToPlay(JsonArray laserCommands, bool &success)
{
    unsigned int laserClusterIndex = 0;
    for (JsonObject commandCluster : laserCommands)
    {
        unsigned long elapsedSinceFirstExecution = millis() - _firstExecutionStartedAtMillis;
        unsigned int clusterTimeToSelect = roundDown(elapsedSinceFirstExecution);

        if (commandCluster["timeMs"] == clusterTimeToSelect)
        {
            success = true;
            return commandCluster;
        }

        laserClusterIndex++;
    }

    return JsonObject();
}

void PlaySDFileMode::playCluster(JsonObject cluster)
{
    JsonArray commands = cluster["commands"];
    for (JsonArray arrayCluster : commands)
    {
        for (JsonObject animationPattern : arrayCluster)
        {
            byte r = animationPattern["r"];
            byte g = animationPattern["g"];
            byte b = animationPattern["b"];
            short x = animationPattern["x"];
            short y = animationPattern["y"];

            _laser.sendTo(x, y);
            _laser.setLaserPower(r, g, b);
        }
    }

    _laser.setLaserPower(0, 0, 0);
}

void PlaySDFileMode::execute()
{
    if (SelectedSDCardFilename == "")
    {
        Serial.println("Empty filename");
        return;
    }

    bool executionShouldBeStarted = _firstExecutionStartedAtMillis == 4294967295;
    if (executionShouldBeStarted)
    {
        Serial.println("executionShouldBeStarted");
        _firstExecutionStartedAtMillis = millis();
    }

    long kpps = SelectedSDCardJson["kpps"];
    long duration = SelectedSDCardJson["duration"];
    JsonArray laserCommands = SelectedSDCardJson["laserCommands"];

    bool getCurrentClusterSuccess = false;
    JsonObject clusterToPlay = getCurrentClusterToPlay(laserCommands, getCurrentClusterSuccess);

    if (getCurrentClusterSuccess)
    {
        playCluster(clusterToPlay);
    }

    if (millis() - _firstExecutionStartedAtMillis > duration)
    {
        CurrentLaserMode = LaserMode::NotSelected;
        _firstExecutionStartedAtMillis = 4294967295;
    }
}

void PlaySDFileMode::stop()
{
    _firstExecutionStartedAtMillis = 4294967295;
}

LaserMode PlaySDFileMode::getModeName()
{
    return LaserMode::SDCardMode;
}