#ifndef DAEMON_H
#define DAEMON_H

#include <string>
#include <atomic>
#include <thread>
#include <unistd.h>
#include "logger.h"
#include "monitor.h"
#include "tc_control.h"
#include "nft_control.h"
#include "target.h"
#include "policy_engine.h"
#include "resource_guard.h"
#include "scheduler.h"

class NreDaemon {
public:
    NreDaemon(Logger &logger);
    ~NreDaemon();

    bool start();

    bool stop();

    bool isRunning() const;

    void setMonitorTarget(const std::string &target);
    void setMonitorInterface(const std::string &interface);
    void setMonitorInterval(int seconds);

    ExperimentScheduler& getScheduler() { return scheduler; }

private:
    Logger &log;
    TcControl tcControl;
    NftControl nftControl;
    TargetSelector targetSelector;
    PolicyEngine policyEngine;
    ResourceGuard resourceGuard;
    ExperimentScheduler scheduler;

    std::thread daemonThread;
    std::atomic<bool> running;
    std::string monitorTarget;
    std::string monitorInterface;
    int monitorIntervalSeconds;

    void daemonLoop();
    void monitorAndAct();
};

bool writePidFile(const std::string &path);
bool readPidFile(const std::string &path, pid_t &pid);
bool removePidFile(const std::string &path);

#endif
