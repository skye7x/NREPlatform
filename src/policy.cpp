#include "policy.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>

PolicyManager::PolicyManager(Logger &logger, const std::string &folderName)
    : log(logger), folder(folderName) {
    mkdir(folder.c_str(), 0755);
}

bool PolicyManager::savePolicy(const Policy &policy) {
    std::string path = folder + "/" + policy.name + ".policy";
    std::ofstream file(path);

    if (!file.is_open()) {
        log.log("ERROR: could not save policy to " + path);
        return false;
    }

    file << "name=" << policy.name << "\n";
    file << "logic=" << policy.logic << "\n";
    file << "action=" << policy.action << "\n";
    file << "action_param=" << policy.actionParam << "\n";

    for (size_t i = 0; i < policy.conditions.size() && i < (size_t)MAX_CONDITIONS; ++i) {
        std::string prefix = "condition" + std::to_string(i + 1) + "_";
        file << prefix << "metric=" << policy.conditions[i].metric << "\n";
        file << prefix << "op="     << policy.conditions[i].op     << "\n";
        file << prefix << "value="  << policy.conditions[i].value  << "\n";
    }

    file.close();
    log.log("Policy saved: " + policy.name + " (" + path + ")");
    return true;
}

Policy PolicyManager::loadPolicy(const std::string &name, bool &found) {
    Policy policy;
    policy.name = name;

    std::string path = folder + "/" + name + ".policy";
    std::ifstream file(path);

    if (!file.is_open()) {
        found = false;
        log.log("ERROR: policy not found: " + name);
        return policy;
    }

    Condition slots[MAX_CONDITIONS];

    std::string line;
    while (std::getline(file, line)) {
        size_t splitPos = line.find('=');
        if (splitPos == std::string::npos) continue;

        std::string key = line.substr(0, splitPos);
        std::string value = line.substr(splitPos + 1);

        if (key == "logic") policy.logic = value;
        else if (key == "action") policy.action = value;
        else if (key == "action_param") policy.actionParam = value;
        else if (key.rfind("condition", 0) == 0) {
            
            int index = key[9] - '1'; 
            if (index < 0 || index >= MAX_CONDITIONS) continue;

            if (key.find("_metric") != std::string::npos) slots[index].metric = value;
            else if (key.find("_op") != std::string::npos) slots[index].op = value;
            else if (key.find("_value") != std::string::npos) slots[index].value = std::atof(value.c_str());
        }
    }

    for (int i = 0; i < MAX_CONDITIONS; ++i) {
        if (!slots[i].isEmpty()) {
            policy.conditions.push_back(slots[i]);
        }
    }

    found = true;
    log.log("Policy loaded: " + name);
    return policy;
}

std::vector<std::string> PolicyManager::listPolicyNames() {
    std::vector<std::string> names;

    DIR *dir = opendir(folder.c_str());
    if (!dir) return names;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        std::string suffix = ".policy";

        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            names.push_back(filename.substr(0, filename.size() - suffix.size()));
        }
    }

    closedir(dir);
    return names;
}

std::vector<Policy> PolicyManager::loadAllPolicies() {
    std::vector<Policy> policies;

    for (const std::string &name : listPolicyNames()) {
        bool found = false;
        Policy policy = loadPolicy(name, found);
        if (found) {
            policies.push_back(policy);
        }
    }

    return policies;
}
