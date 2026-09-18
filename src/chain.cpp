#include "chain.h"
#include "experiment.h"
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>

ChainManager::ChainManager(Logger &logger, const std::string &folderName)
    : log(logger), folder(folderName) {
    mkdir(folder.c_str(), 0755);
}

bool ChainManager::saveChain(const Chain &chain) {
    std::string path = folder + "/" + chain.name + ".chain";
    std::ofstream file(path);

    if (!file.is_open()) {
        log.log("ERROR: could not save chain to " + path);
        return false;
    }

    file << "name=" << chain.name << "\n";
    file << "step_count=" << chain.steps.size() << "\n";

    for (size_t i = 0; i < chain.steps.size() && i < (size_t)MAX_CHAIN_STEPS; ++i) {
        std::string prefix = "step" + std::to_string(i + 1) + "_";
        const ExperimentProfile &s = chain.steps[i];

        file << prefix << "target_ip=" << s.targetIp << "\n";
        file << prefix << "target_mac=" << s.targetMac << "\n";
        file << prefix << "latency_ms=" << s.latencyMs << "\n";
        file << prefix << "jitter_ms=" << s.jitterMs << "\n";
        file << prefix << "packet_loss_pc=" << s.packetLossPc << "\n";
        file << prefix << "download_kbit=" << s.downloadKbit << "\n";
        file << prefix << "upload_kbit=" << s.uploadKbit << "\n";
        file << prefix << "duration_seconds=" << s.durationSeconds << "\n";
    }

    file.close();
    log.log("Chain saved: " + chain.name + " (" + std::to_string(chain.steps.size()) + " steps)");
    return true;
}

Chain ChainManager::loadChain(const std::string &name, bool &found) {
    Chain chain;
    chain.name = name;

    std::string path = folder + "/" + name + ".chain";
    std::ifstream file(path);

    if (!file.is_open()) {
        found = false;
        log.log("ERROR: chain not found: " + name);
        return chain;
    }

    int stepCount = 0;
    ExperimentProfile slots[MAX_CHAIN_STEPS];

    std::string line;
    while (std::getline(file, line)) {
        size_t splitPos = line.find('=');
        if (splitPos == std::string::npos) continue;

        std::string key = line.substr(0, splitPos);
        std::string value = line.substr(splitPos + 1);

        if (key == "step_count") {
            stepCount = std::atoi(value.c_str());
            continue;
        }
        if (key.rfind("step", 0) != 0) continue;

        int index = key[4] - '1';
        if (index < 0 || index >= MAX_CHAIN_STEPS) continue;

        if (key.find("_target_ip") != std::string::npos) slots[index].targetIp = value;
        else if (key.find("_target_mac") != std::string::npos) slots[index].targetMac = value;
        else if (key.find("_latency_ms") != std::string::npos) slots[index].latencyMs = std::atoi(value.c_str());
        else if (key.find("_jitter_ms") != std::string::npos) slots[index].jitterMs = std::atoi(value.c_str());
        else if (key.find("_packet_loss_pc") != std::string::npos) slots[index].packetLossPc = std::atof(value.c_str());
        else if (key.find("_download_kbit") != std::string::npos) slots[index].downloadKbit = std::atoi(value.c_str());
        else if (key.find("_upload_kbit") != std::string::npos) slots[index].uploadKbit = std::atoi(value.c_str());
        else if (key.find("_duration_seconds") != std::string::npos) slots[index].durationSeconds = std::atoi(value.c_str());
    }

    for (int i = 0; i < stepCount && i < MAX_CHAIN_STEPS; ++i) {
        slots[i].name = chain.name + " - step " + std::to_string(i + 1);
        chain.steps.push_back(slots[i]);
    }

    found = true;
    log.log("Chain loaded: " + name + " (" + std::to_string(chain.steps.size()) + " steps)");
    return chain;
}

std::vector<std::string> ChainManager::listChainNames() {
    std::vector<std::string> names;

    DIR *dir = opendir(folder.c_str());
    if (!dir) return names;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        std::string suffix = ".chain";

        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            names.push_back(filename.substr(0, filename.size() - suffix.size()));
        }
    }

    closedir(dir);
    return names;
}

bool ChainManager::executeChain(const Chain &chain,
                                 const std::string &interfaceName,
                                 TcControl &tcControl,
                                 TargetSelector &targetSelector,
                                 ResourceGuard *guard) {
    std::cout << "\n=== Executing Chain: " << chain.name
              << " (" << chain.steps.size() << " steps) ===\n" << std::endl;

    log.log("Chain execution started: " + chain.name);

    for (size_t i = 0; i < chain.steps.size(); ++i) {
        const ExperimentProfile &step = chain.steps[i];
        std::cout << "--- Step " << (i + 1) << "/" << chain.steps.size()
                  << ": " << step.name << " ---" << std::endl;

        log.log("Chain step " + std::to_string(i + 1) + ": " + step.name);

        std::string resolvedIp = step.targetIp;
        if (resolvedIp.empty() && !step.targetMac.empty()) {
            resolvedIp = targetSelector.resolveMacToIp(step.targetMac);
            if (resolvedIp.empty()) {
                log.log("WARNING: could not resolve MAC for step " + std::to_string(i + 1) + ", skipping.");
                std::cout << "  Skipping: could not resolve target." << std::endl;
                continue;
            }
        }

        Experiment experiment(step.name,
                              interfaceName,
                              resolvedIp,
                              step.targetMac,
                              step.latencyMs,
                              step.jitterMs,
                              step.packetLossPc,
                              step.downloadKbit,
                              step.uploadKbit,
                              step.durationSeconds,
                              log,
                              tcControl,
                              targetSelector,
                              guard);

        experiment.printSummary();
        MetricsResult metrics = experiment.run();

        if (experiment.wasSuccessful()) {
            std::cout << "  Step " << (i + 1) << " completed successfully." << std::endl;
            NetworkMonitor::printResult(metrics);
        } else {
            std::cout << "  Step " << (i + 1) << " failed or was aborted." << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << "=== Chain execution finished: " << chain.name << " ===\n" << std::endl;
    log.log("Chain execution finished: " + chain.name);
    return true;
}
