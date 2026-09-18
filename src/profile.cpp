#include "profile.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>   
#include <dirent.h>     

ProfileManager::ProfileManager(Logger &logger, const std::string &folderName)
    : log(logger), folder(folderName) {
    
    mkdir(folder.c_str(), 0755);
}

bool ProfileManager::saveProfile(const ExperimentProfile &profile) {
    std::string path = folder + "/" + profile.name + ".profile";
    std::ofstream file(path);

    if (!file.is_open()) {
        log.log("ERROR: could not save profile to " + path);
        return false;
    }

    file << "name=" << profile.name << "\n";
    file << "target_ip=" << profile.targetIp << "\n";
    file << "target_mac=" << profile.targetMac << "\n";
    file << "latency_ms=" << profile.latencyMs << "\n";
    file << "jitter_ms=" << profile.jitterMs << "\n";
    file << "packet_loss_pc=" << profile.packetLossPc << "\n";
    file << "download_kbit=" << profile.downloadKbit << "\n";
    file << "upload_kbit=" << profile.uploadKbit << "\n";
    file << "duration_seconds=" << profile.durationSeconds << "\n";

    file.close();
    log.log("Profile saved: " + profile.name + " (" + path + ")");
    return true;
}

ExperimentProfile ProfileManager::loadProfile(const std::string &name, bool &found) {
    ExperimentProfile profile;
    profile.name = name;

    std::string path = folder + "/" + name + ".profile";
    std::ifstream file(path);

    if (!file.is_open()) {
        found = false;
        log.log("ERROR: profile not found: " + name);
        return profile;
    }

    std::string line;
    while (std::getline(file, line)) {
        
        size_t splitPos = line.find('=');
        if (splitPos == std::string::npos) continue;

        std::string key = line.substr(0, splitPos);
        std::string value = line.substr(splitPos + 1);

        if (key == "target_ip") profile.targetIp = value;
        else if (key == "target_mac") profile.targetMac = value;
        else if (key == "latency_ms") profile.latencyMs = std::atoi(value.c_str());
        else if (key == "jitter_ms") profile.jitterMs = std::atoi(value.c_str());
        else if (key == "packet_loss_pc") profile.packetLossPc = std::atof(value.c_str());
        else if (key == "download_kbit") profile.downloadKbit = std::atoi(value.c_str());
        else if (key == "upload_kbit") profile.uploadKbit = std::atoi(value.c_str());
        else if (key == "duration_seconds") profile.durationSeconds = std::atoi(value.c_str());
    }

    found = true;
    log.log("Profile loaded: " + name);
    return profile;
}

std::vector<std::string> ProfileManager::listProfiles() {
    std::vector<std::string> names;

    DIR *dir = opendir(folder.c_str());
    if (!dir) {
        return names; 
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;

        std::string suffix = ".profile";
        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            std::string name = filename.substr(0, filename.size() - suffix.size());
            names.push_back(name);
        }
    }

    closedir(dir);
    return names;
}
