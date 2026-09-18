#ifndef RESOURCE_GUARD_H
#define RESOURCE_GUARD_H

#include <string>
#include "logger.h"

class ResourceGuard {
public:
    ResourceGuard(Logger &logger,
                  int maxDurationSeconds = 600,   
                  int maxConcurrentExperiments = 3,
                  const std::string &runningFolder = "running");

    bool checkDuration(int durationSeconds) const;

    bool tryAcquireSlot(const std::string &experimentName, std::string &slotIdOut);

    void releaseSlot(const std::string &slotId);

    int countRunningExperiments() const;

private:
    Logger &log;
    int maxDurationSeconds;
    int maxConcurrentExperiments;
    std::string runningFolder;
};

#endif
