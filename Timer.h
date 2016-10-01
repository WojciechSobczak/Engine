#pragma once
#include "stdafx.h"

class Timer {

public:
	static float getTotalTime(); // in seconds
	static float getDeltaTime() {
		return deltaTime;
	}; // in seconds

	static void init();
	static void reset(); // Call before message loop.
	static void start() {
		stopped = false;
	}; // Call when unpaused.
	static void stop() {
		stopped = true;
	}; // Call when paused.
	static void tick(); // Call every frame.

private:

	static double secondsPerCount;
	static double deltaTime;

	static __int64 baseTime;
	static __int64 pausedTime;
	static __int64 stopTime;
	static __int64 previousTime;
	static __int64 currentTime;

	static bool stopped;
};