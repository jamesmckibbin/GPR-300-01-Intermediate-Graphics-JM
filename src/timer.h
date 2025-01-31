#pragma once

#include "gconst.h"

class Timer
{
public:
    Timer();
    float GetFrameDelta();
private:
    float timerFrequency = 0.0;
    long long lastFrameTime = 0;
    long long lastSecond = 0;
    float frameDelta = 0;
    int fps = 0;
};