#ifndef FAILURE_EXPERIMENT_H
#define FAILURE_EXPERIMENT_H

#include <string>
#include "logger.h"
#include "nft_control.h"
#include "target.h"
#include "resource_guard.h"

enum class FailureType {
    DNS_FAILURE,
    CONNECTION_INTERRUPTION
};

class FailureExperiment {
public:
    FailureExperiment(const std::string &name,
                       FailureType type,
                       const std::string &targetIp,
                       const std::string &targetMac,
                       int durationSeconds,
                       Logger &logger,
                       NftControl &nftControl,
                       TargetSelector &targetSelector,
                       ResourceGuard *resourceGuard = nullptr);

    void run();

    void printSummary() const;

    bool wasSuccessful() const { return success; }

private:
    std::string name;
    FailureType type;
    std::string targetIp;
    std::string targetMac;
    int durationSeconds;

    Logger &log;
    NftControl &nft;
    TargetSelector &targetSelector;
    ResourceGuard *guard;

    bool success = false;
    std::string resourceSlotId;
};

#endif
