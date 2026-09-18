#ifndef PROFILE_H
#define PROFILE_H

#include <string>
#include <vector>
#include "logger.h"

struct ExperimentProfile {
    std::string name;
    std::string targetIp;      
    std::string targetMac;     
    int latencyMs = 0;
    int jitterMs = 0;
    double packetLossPc = 0.0;
    int downloadKbit = 0;      
    int uploadKbit = 0;        
    int durationSeconds = 60;
};

class ProfileManager {
public:
    ProfileManager(Logger &logger, const std::string &folder = "profiles");

    bool saveProfile(const ExperimentProfile &profile);

    ExperimentProfile loadProfile(const std::string &name, bool &found);

    std::vector<std::string> listProfiles();

private:
    Logger &log;
    std::string folder;
};

#endif
