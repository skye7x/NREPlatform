#include "resource_guard.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <unistd.h>   
#include <ctime>

ResourceGuard::ResourceGuard(Logger &logger,
                              int maxDuration,
                              int maxConcurrent,
                              const std::string &folder)
    : log(logger),
      maxDurationSeconds(maxDuration),
      maxConcurrentExperiments(maxConcurrent),
      runningFolder(folder) {

    mkdir(runningFolder.c_str(), 0755);
}

bool ResourceGuard::checkDuration(int durationSeconds) const {
    if (durationSeconds > maxDurationSeconds) {
        log.log("REJECTED: requested duration (" + std::to_string(durationSeconds) +
                 "s) exceeds the maximum allowed (" + std::to_string(maxDurationSeconds) + "s).");
        return false;
    }
    return true;
}

int ResourceGuard::countRunningExperiments() const {
    int count = 0;

    DIR *dir = opendir(runningFolder.c_str());
    if (!dir) return 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        std::string suffix = ".lock";

        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

bool ResourceGuard::tryAcquireSlot(const std::string &experimentName, std::string &slotIdOut) {
    int currentCount = countRunningExperiments();

    if (currentCount >= maxConcurrentExperiments) {
        log.log("REJECTED: maximum concurrent experiments (" +
                 std::to_string(maxConcurrentExperiments) + ") already reached.");
        return false;
    }

    slotIdOut = "pid" + std::to_string(getpid()) + "_" + std::to_string(time(nullptr)) + ".lock";

    std::ofstream lockFile(runningFolder + "/" + slotIdOut);
    if (!lockFile.is_open()) {
        log.log("ERROR: could not create resource lock file.");
        return false;
    }
    lockFile << experimentName << "\n";
    lockFile.close();

    log.log("Resource slot acquired for: " + experimentName +
             " (" + std::to_string(currentCount + 1) + "/" +
             std::to_string(maxConcurrentExperiments) + " in use)");
    return true;
}

void ResourceGuard::releaseSlot(const std::string &slotId) {
    if (slotId.empty()) return;

    std::string path = runningFolder + "/" + slotId;
    remove(path.c_str());
    log.log("Resource slot released.");
}
