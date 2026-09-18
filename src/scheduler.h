#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "logger.h"
#include "profile.h"

struct ScheduledExperiment {
    int id = 0;
    std::string profileName;
    std::string interfaceName;
    std::string targetIp;
    std::string targetMac;
    int latencyMs = 0;
    int jitterMs = 0;
    double packetLossPc = 0.0;
    int downloadKbit = 0;
    int uploadKbit = 0;
    int durationSeconds = 60;
    std::string scheduledTime;     
    int delaySeconds = 0;          
    bool repeatDaily = false;
    bool enabled = true;
};

class ExperimentScheduler {
public:
    ExperimentScheduler(Logger &logger, const std::string &folder = "schedules");
    ~ExperimentScheduler();

    void start();

    void stop();

    bool addSchedule(const ScheduledExperiment &schedule);

    bool removeSchedule(int id);

    std::vector<ScheduledExperiment> listSchedules();

    bool saveSchedule(const ScheduledExperiment &schedule);
    bool loadSchedules();
    bool saveSchedules();

    using ScheduleCallback = std::function<bool(const ScheduledExperiment &)>;
    void setCallback(ScheduleCallback cb);

private:
    Logger &log;
    std::string folder;
    std::vector<ScheduledExperiment> schedules;
    std::thread schedulerThread;
    std::atomic<bool> running;
    std::mutex scheduleMutex;
    ScheduleCallback callback;
    int nextId;

    void schedulerLoop();
    bool shouldTrigger(const ScheduledExperiment &schedule);
    std::string getCurrentTime();
};

#endif
