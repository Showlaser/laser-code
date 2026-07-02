#ifndef POINT_H
#define POINT_H

#include <Arduino.h>

struct point
{
    char Uuid[37];        // 36-char UUID + null terminator
    char PatternUuid[37]; // 36-char UUID + null terminator
    int X;
    int Y;
    byte RedLaserPowerPwm;
    byte GreenLaserPowerPwm;
    byte BlueLaserPowerPwm;
};