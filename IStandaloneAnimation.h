#ifndef ISTANDALONEANIMATION_H
#define ISTANDALONEANIMATION_H

#include "GlobalConfig.h"

class IStandaloneAnimation
{
public:
    virtual void execute(unsigned long animationStartedAtMillis, int timePerAnimationInSeconds);
    virtual String getAnimationName();
    virtual ~IStandaloneAnimation() {}
};

#endif