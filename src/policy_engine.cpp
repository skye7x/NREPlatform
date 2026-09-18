#include "policy_engine.h"
#include <iostream>
#include <cstdlib>
#include <fstream>

PolicyEngine::PolicyEngine(Logger &logger, NftControl &nftControl, TcControl &tcControl)
    : log(logger), nft(nftControl), tc(tcControl) {
}

void PolicyEngine::setExperimentCallbacks(ExperimentStartCallback startCb, ExperimentStopCallback stopCb) {
    startExperimentCb = startCb;
    stopExperimentCb = stopCb;
}

bool PolicyEngine::evaluatePolicy(const Policy &policy, const MetricsResult &metrics) {
    if (policy.conditions.empty()) {
        return false;
    }

    if (policy.logic == "OR") {
        for (const Condition &condition : policy.conditions) {
            if (condition.evaluate(metrics)) {
                return true;
            }
        }
        return false;
    }

    for (const Condition &condition : policy.conditions) {
        if (!condition.evaluate(metrics)) {
            return false;
        }
    }
    return true;
}

void PolicyEngine::executeAction(const Policy &policy, const std::string &defaultInterface) {
    const std::string &action = policy.action;

    if (action == "trigger_alert" || action == "generate_alert") {
        std::cout << "\n*** ALERT: policy '" << policy.name << "' triggered! ***\n" << std::endl;
        log.log("ALERT: policy '" + policy.name + "' triggered.");

    } else if (action == "create_log_entry") {
        log.log("Policy log entry: '" + policy.name + "' conditions were met.");

    } else if (action == "block_traffic") {
        if (policy.actionParam.empty()) {
            log.log("WARNING: policy '" + policy.name + "' action=block_traffic but no target IP set.");
            return;
        }
        nft.setupTable();
        nft.blockConnection(policy.actionParam);
        log.log("Policy '" + policy.name + "' blocked traffic for " + policy.actionParam);

    } else if (action == "allow_traffic") {
        nft.teardownTable();
        log.log("Policy '" + policy.name + "' restored traffic (allow_traffic).");

    } else if (action == "enable_qos") {
        
        int qosLimitKbit = 5000; 
        std::string qosTarget;

        size_t colonPos = policy.actionParam.find(':');
        if (colonPos != std::string::npos) {
            qosTarget = policy.actionParam.substr(0, colonPos);
            qosLimitKbit = std::atoi(policy.actionParam.substr(colonPos + 1).c_str());
        } else if (!policy.actionParam.empty()) {
            
            int parsed = std::atoi(policy.actionParam.c_str());
            if (parsed > 0) {
                qosLimitKbit = parsed;
            }
        }

        tc.applyShapedNetem(defaultInterface, qosTarget, "dst", 0, 0, 0.0, qosLimitKbit);
        log.log("Policy '" + policy.name + "' enabled QoS (bandwidth capped at "
                 + std::to_string(qosLimitKbit) + " kbit/s"
                 + (qosTarget.empty() ? " on " + defaultInterface : " for " + qosTarget) + ").");
        std::cout << "QoS enabled: " << qosLimitKbit << " kbit/s" << std::endl;

    } else if (action == "change_dns") {
        
        if (policy.actionParam.empty()) {
            log.log("WARNING: policy '" + policy.name + "' action=change_dns but no params set.");
            return;
        }
        size_t colonPos = policy.actionParam.find(':');
        if (colonPos == std::string::npos) {
            log.log("WARNING: policy '" + policy.name + "' change_dns format should be target_ip:dns_server.");
            return;
        }
        std::string targetIp = policy.actionParam.substr(0, colonPos);
        std::string dnsServer = policy.actionParam.substr(colonPos + 1);

        nft.setupTable();

        std::string rule = "sudo nft add rule inet nre_platform block ip saddr " + targetIp +
            " udp dport 53 dnat to " + dnsServer;
        std::system(rule.c_str());

        std::string ruleTcp = "sudo nft add rule inet nre_platform block ip saddr " + targetIp +
            " tcp dport 53 dnat to " + dnsServer;
        std::system(ruleTcp.c_str());

        log.log("Policy '" + policy.name + "' changed DNS for " + targetIp + " to " + dnsServer);
        std::cout << "DNS changed: " << targetIp << " -> " << dnsServer << std::endl;

    } else if (action == "restart_service") {
        
        if (policy.actionParam.empty()) {
            log.log("WARNING: policy '" + policy.name + "' action=restart_service but no service name set.");
            return;
        }
        std::string cmd = "sudo /etc/init.d/" + policy.actionParam + " restart";
        int result = std::system(cmd.c_str());
        if (result == 0) {
            log.log("Policy '" + policy.name + "' restarted service: " + policy.actionParam);
            std::cout << "Service restarted: " << policy.actionParam << std::endl;
        } else {
            log.log("WARNING: policy '" + policy.name + "' failed to restart service: " + policy.actionParam);
            std::cout << "Failed to restart service: " << policy.actionParam << std::endl;
        }

    } else if (action == "change_network_profile") {
        
        if (policy.actionParam.empty()) {
            log.log("WARNING: policy '" + policy.name + "' action=change_network_profile but no profile name set.");
            return;
        }
        
        log.log("Policy '" + policy.name + "' requests profile change to: " + policy.actionParam);
        std::cout << "Profile change requested: " << policy.actionParam << std::endl;

        std::ofstream marker("pending_profile_change.txt");
        if (marker.is_open()) {
            marker << policy.actionParam << "\n";
            marker.close();
        }

    } else if (action == "start_experiment") {
        
        if (policy.actionParam.empty()) {
            log.log("WARNING: policy '" + policy.name + "' action=start_experiment but no profile name set.");
            return;
        }
        if (startExperimentCb) {
            ExperimentProfile profile;
            profile.name = policy.actionParam;
            log.log("Policy '" + policy.name + "' starting experiment: " + policy.actionParam);
            std::cout << "Starting experiment: " << policy.actionParam << std::endl;
            startExperimentCb(profile, defaultInterface);
        } else {
            log.log("Policy '" + policy.name + "' wants to start experiment but no callback set.");
            std::cout << "Cannot start experiment: callback not configured." << std::endl;
        }

    } else if (action == "stop_experiment") {
        
        if (stopExperimentCb) {
            log.log("Policy '" + policy.name + "' stopping current experiment.");
            std::cout << "Stopping current experiment." << std::endl;
            stopExperimentCb();
        } else {
            log.log("Policy '" + policy.name + "' wants to stop experiment but no callback set.");
            std::cout << "Cannot stop experiment: callback not configured." << std::endl;
        }

    } else {
        log.log("WARNING: policy '" + policy.name + "' has unknown action '" + action + "'.");
    }
}

int PolicyEngine::evaluateAndRun(const std::vector<Policy> &policies,
                                   const MetricsResult &metrics,
                                   const std::string &defaultInterface) {
    int triggeredCount = 0;

    for (const Policy &policy : policies) {
        bool matched = evaluatePolicy(policy, metrics);

        if (matched) {
            log.log("Policy matched: " + policy.name);
            executeAction(policy, defaultInterface);
            triggeredCount++;
        }
    }

    return triggeredCount;
}
