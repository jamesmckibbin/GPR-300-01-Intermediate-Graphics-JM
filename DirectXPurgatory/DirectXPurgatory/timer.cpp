#include "timer.h"

Timer::Timer()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);

    timerFrequency = float(li.QuadPart) / 1000.0f;

    QueryPerformanceCounter(&li);
    lastFrameTime = li.QuadPart;
}

float Timer::GetFrameDelta()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    frameDelta = float(li.QuadPart - lastFrameTime) / timerFrequency;
    if (frameDelta > 0)
        fps = (int)(1000 / frameDelta);
    lastFrameTime = li.QuadPart;
    return frameDelta;
}