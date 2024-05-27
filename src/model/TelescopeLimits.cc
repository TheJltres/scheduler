#ifndef SCHEDULER_TELESCOPE_CONFIGURATION
#define SCHEDULER_TELESCOPE_CONFIGURATION

#include <climits>

struct TelescopeLimits {
    double MinHeight;
    double MinLunarDistance;
    double MinDecNord;
    double MinDecSouth;
    double MountHA;
};

#endif
