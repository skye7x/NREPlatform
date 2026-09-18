#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <string>
#include "tc_control.h"
#include "logger.h"
#include "target.h"
#include "monitor.h"
#include "resource_guard.h"

class Experiment {
public:
    Experiment(const std::string &name,
               const std::string &lanInterface,
               const std::string &targetIp,
               const std::string &targetMac,
               int latencyMs,
               int jitterMs,
               double packetLossPc,
               int downloadKbit,
               int uploadKbit,
               int durationSeconds,
               Logger &logger,
               TcControl &tcControl,
               TargetSelector &targetSelector,
               ResourceGuard *resourceGuard = nullptr);

    MetricsResult run();

    void printSummary() const;

    bool wasSuccessful() const { return success; }

private:
    std::string name;
    std::string lanInterface;
    std::string targetIp;
    std::string targetMac;
    int latencyMs;
    int jitterMs;
    double packetLossPc;
    int downloadKbit;
    int uploadKbit;
    int durationSeconds;

    Logger &log;
    TcControl &tc;
    TargetSelector &targetSelector;
    ResourceGuard *guard;

    const std::string ifbInterface = "ifb0";

    bool uploadShapingActive = false;

    bool success = false;

    std::string resourceSlotId;
};

#endif
