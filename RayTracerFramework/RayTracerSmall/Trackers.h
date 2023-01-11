#pragma once
#include "Tracker.h"

class Trackers
{
public:
	static Tracker* GetDefaultTracker();
	static Tracker* GetSphereTracker();
	static Tracker* GetMemoryPoolTracker();
	static Tracker* GetThreadingTracker();

private:
	static Tracker defaultTracker;
	static Tracker sphereTracker;
	static Tracker memorypoolTracker;
	static Tracker threadingTracker;
};

