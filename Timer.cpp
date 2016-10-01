#include "stdafx.h"
#include "Timer.h"
#include <windows.h>



double Timer::secondsPerCount = 0;
double Timer::deltaTime = -1;
__int64 Timer::baseTime = 0;
__int64 Timer::pausedTime = 0;
__int64 Timer::stopTime = 0;
__int64 Timer::previousTime = 0;
__int64 Timer::currentTime = 0;
bool Timer::stopped = false;

void Timer::reset() {
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*) &currTime);

	Timer::baseTime = currTime;
	Timer::previousTime = currTime;
	Timer::stopTime = 0;
	Timer::stopped = false;
}

void Timer::init() {
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*) &countsPerSec);
	Timer::secondsPerCount = 1.0 / (double) countsPerSec;
}

void Timer::tick() {
	if (stopped) {
		deltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*) &currTime);
	previousTime = currentTime = currTime;

	if (deltaTime < 0.0) {
		deltaTime = 0.0;
	}
}

float Timer::getTotalTime() {
	if (stopped) {
		return (float) (((stopTime - pausedTime) - baseTime) * secondsPerCount);
	} else {
		return (float) (((currentTime - pausedTime) - baseTime) * secondsPerCount);
	}
}