#include "Trackers.h"

Tracker Trackers::defaultTracker;
Tracker Trackers::sphereTracker;
Tracker Trackers::memorypoolTracker;
Tracker Trackers::threadingTracker;

Tracker* Trackers::GetDefaultTracker()
{
	return &defaultTracker;
}

Tracker* Trackers::GetSphereTracker()
{
	return &sphereTracker;
}

Tracker* Trackers::GetMemoryPoolTracker()
{
	return &memorypoolTracker;
}

Tracker* Trackers::GetThreadingTracker()
{
	return &threadingTracker;
}