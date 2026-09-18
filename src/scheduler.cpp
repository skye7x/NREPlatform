#include "scheduler.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <iostream>

ExperimentScheduler::ExperimentScheduler(Logger &logger, const std::string &folderName)
    : log(logger), folder(folderName), running(false), nextId(1) {
    mkdir(folder.c_str(), 0755);
    loadSchedules();
}

ExperimentScheduler::~ExperimentScheduler() {
    stop();
}

void ExperimentScheduler::start() {
    if (running) return;
    running = true;
    schedulerThread = std::thread(&ExperimentScheduler::schedulerLoop, this);
    log.log("Scheduler started.");
    std::cout << "Scheduler started." << std::endl;
}

void ExperimentScheduler::stop() {
    running = false;
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
    log.log("Scheduler stopped.");
}

void ExperimentScheduler::setCallback(ScheduleCallback cb) {
    callback = cb;
}

bool ExperimentScheduler::addSchedule(const ScheduledExperiment &schedule) {
    std::lock_guard<std::mutex> lock(scheduleMutex);

    ScheduledExperiment newSchedule = schedule;
    newSchedule.id = nextId++;
    schedules.push_back(newSchedule);

    saveSchedules();

    log.log("Schedule added: #" + std::to_string(newSchedule.id) +
            " profile=" + newSchedule.profileName +
            " time=" + newSchedule.scheduledTime);
    std::cout << "Schedule added: #" << newSchedule.id << std::endl;

    return true;
}

bool ExperimentScheduler::removeSchedule(int id) {
    std::lock_guard<std::mutex> lock(scheduleMutex);

    for (auto it = schedules.begin(); it != schedules.end(); ++it) {
        if (it->id == id) {
            schedules.erase(it);
            saveSchedules();
            log.log("Schedule removed: #" + std::to_string(id));
            std::cout << "Schedule removed: #" << id << std::endl;
            return true;
        }
    }

    std::cout << "Schedule #" << id << " not found." << std::endl;
    return false;
}

std::vector<ScheduledExperiment> ExperimentScheduler::listSchedules() {
    std::lock_guard<std::mutex> lock(scheduleMutex);
    return schedules;
}

bool ExperimentScheduler::saveSchedule(const ScheduledExperiment &schedule) {
    std::string path = folder + "/schedule_" + std::to_string(schedule.id) + ".txt";
    std::ofstream file(path);

    if (!file.is_open()) {
        log.log("ERROR: could not save schedule to " + path);
        return false;
    }

    file << "id=" << schedule.id << "\n";
    file << "profile_name=" << schedule.profileName << "\n";
    file << "interface_name=" << schedule.interfaceName << "\n";
    file << "target_ip=" << schedule.targetIp << "\n";
    file << "target_mac=" << schedule.targetMac << "\n";
    file << "latency_ms=" << schedule.latencyMs << "\n";
    file << "jitter_ms=" << schedule.jitterMs << "\n";
    file << "packet_loss_pc=" << schedule.packetLossPc << "\n";
    file << "download_kbit=" << schedule.downloadKbit << "\n";
    file << "upload_kbit=" << schedule.uploadKbit << "\n";
    file << "duration_seconds=" << schedule.durationSeconds << "\n";
    file << "scheduled_time=" << schedule.scheduledTime << "\n";
    file << "delay_seconds=" << schedule.delaySeconds << "\n";
    file << "repeat_daily=" << (schedule.repeatDaily ? "1" : "0") << "\n";
    file << "enabled=" << (schedule.enabled ? "1" : "0") << "\n";

    file.close();
    return true;
}

bool ExperimentScheduler::loadSchedules() {
    std::lock_guard<std::mutex> lock(scheduleMutex);

    DIR *dir = opendir(folder.c_str());
    if (!dir) return false;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.find("schedule_") != 0) continue;
        if (filename.find(".txt") == std::string::npos) continue;

        std::string path = folder + "/" + filename;
        std::ifstream file(path);
        if (!file.is_open()) continue;

        ScheduledExperiment schedule;
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (key == "id") schedule.id = std::atoi(value.c_str());
            else if (key == "profile_name") schedule.profileName = value;
            else if (key == "interface_name") schedule.interfaceName = value;
            else if (key == "target_ip") schedule.targetIp = value;
            else if (key == "target_mac") schedule.targetMac = value;
            else if (key == "latency_ms") schedule.latencyMs = std::atoi(value.c_str());
            else if (key == "jitter_ms") schedule.jitterMs = std::atoi(value.c_str());
            else if (key == "packet_loss_pc") schedule.packetLossPc = std::atof(value.c_str());
            else if (key == "download_kbit") schedule.downloadKbit = std::atoi(value.c_str());
            else if (key == "upload_kbit") schedule.uploadKbit = std::atoi(value.c_str());
            else if (key == "duration_seconds") schedule.durationSeconds = std::atoi(value.c_str());
            else if (key == "scheduled_time") schedule.scheduledTime = value;
            else if (key == "delay_seconds") schedule.delaySeconds = std::atoi(value.c_str());
            else if (key == "repeat_daily") schedule.repeatDaily = (value == "1");
            else if (key == "enabled") schedule.enabled = (value == "1");
        }

        if (schedule.id >= nextId) nextId = schedule.id + 1;
        schedules.push_back(schedule);
    }

    closedir(dir);
    return true;
}

bool ExperimentScheduler::saveSchedules() {
    for (const auto &schedule : schedules) {
        saveSchedule(schedule);
    }
    return true;
}

std::string ExperimentScheduler::getCurrentTime() {
    time_t now = time(nullptr);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M", localtime(&now));
    return std::string(buffer);
}

bool ExperimentScheduler::shouldTrigger(const ScheduledExperiment &schedule) {
    if (!schedule.enabled) return false;
    if (!callback) return false;

    std::string currentTime = getCurrentTime();

    if (schedule.delaySeconds > 0) {
        
        return false;
    }

    if (!schedule.scheduledTime.empty()) {
        if (schedule.scheduledTime == currentTime) {
            return true;
        }
    }

    return false;
}

void ExperimentScheduler::schedulerLoop() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(scheduleMutex);

            for (auto &schedule : schedules) {
                if (shouldTrigger(schedule)) {
                    log.log("Scheduler triggering: #" + std::to_string(schedule.id) +
                            " profile=" + schedule.profileName);
                    std::cout << "\n[SCHEDULER] Triggering schedule #" << schedule.id
                              << ": " << schedule.profileName << std::endl;

                    if (callback) {
                        bool success = callback(schedule);
                        if (!success) {
                            log.log("WARNING: schedule #" + std::to_string(schedule.id) +
                                    " execution failed.");
                        }
                    }

                    if (!schedule.repeatDaily) {
                        schedule.enabled = false;
                        saveSchedules();
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}
