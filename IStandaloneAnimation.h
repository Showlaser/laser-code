#ifndef ISTANDALONEANIMATION_H
#define ISTANDALONEANIMATION_H

#include "GlobalConfig.h"
#include "Settings.h"

class IStandaloneAnimation
{
public:
    virtual void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);
    virtual String getAnimationName();
    virtual ~IStandaloneAnimation() {}
};

#endif