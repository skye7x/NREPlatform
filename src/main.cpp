#include <iostream>
#include <string>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#include "logger.h"
#include "tc_control.h"
#include "target.h"
#include "experiment.h"
#include "profile.h"
#include "nft_control.h"
#include "failure_experiment.h"
#include "policy.h"
#include "policy_engine.h"
#include "history.h"
#include "time_utils.h"
#include "resource_guard.h"
#include "chain.h"
#include "scheduler.h"
#include "daemon.h"
#include "api.h"
#include "audit_log.h"
#include "permission.h"
#include "presets.h"
#include "test_automation.h"
#include "storage.h"

void printUsage() {
    std::cout << "NREPlatform - Network Resilience Experimentation Platform\n\n";
    std::cout << "Usage:\n";
    std::cout << "  nre start --iface <name> [--target-ip <ip> | --target-mac <mac>]\n";
    std::cout << "            --latency <ms> --jitter <ms> --loss <percent>\n";
    std::cout << "            --download <kbit> --upload <kbit> --duration <seconds>\n";
    std::cout << "            [--name <text>]\n\n";
    std::cout << "  nre profile save  --profile-name <name> [same flags as start, no --iface]\n";
    std::cout << "  nre profile start --profile-name <name> --iface <name>\n";
    std::cout << "  nre profile list\n\n";
    std::cout << "  nre dns-failure --target-ip <ip> | --target-mac <mac> --duration <seconds> [--name <text>]\n";
    std::cout << "  nre disconnect  --target-ip <ip> | --target-mac <mac> --duration <seconds> [--name <text>]\n\n";
    std::cout << "  nre policy save --policy-name <name> --logic AND|OR --action <name> [--action-param <text>]\n";
    std::cout << "                  --cond1-metric <metric> --cond1-op <op> --cond1-value <n>\n";
    std::cout << "  nre policy list\n";
    std::cout << "  nre policy test --policy-name <name> --latency <n> --loss <n> --jitter <n> --bandwidth <n>\n\n";
    std::cout << "  nre history list [--limit <n>]\n";
    std::cout << "  nre history export --format json|csv --output <path>\n\n";
    std::cout << "  nre chain save --chain-name <name> --profile1 <p1> --profile2 <p2> ...\n";
    std::cout << "  nre chain list\n";
    std::cout << "  nre chain start --chain-name <name> --iface <name>\n\n";
    std::cout << "  nre preset list\n";
    std::cout << "  nre preset info --preset-name <name>\n";
    std::cout << "  nre preset export --preset-name <name> --output <path.json>\n";
    std::cout << "  nre preset export-all --output <path.json>\n";
    std::cout << "  nre preset import --input <path.json>\n\n";
    std::cout << "  nre schedule add --profile-name <name> --iface <name> --time <HH:MM> [--repeat]\n";
    std::cout << "  nre schedule list\n";
    std::cout << "  nre schedule remove --id <n>\n\n";
    std::cout << "  nre daemon start [--target <ip>] [--iface <name>] [--interval <seconds>]\n";
    std::cout << "  nre daemon stop\n";
    std::cout << "  nre daemon status\n\n";
    std::cout << "  nre api start [--port <number>]\n";
    std::cout << "  nre api stop\n\n";
    std::cout << "  nre user add --username <name> --password <pass> --level viewer|operator|admin\n";
    std::cout << "  nre user list\n";
    std::cout << "  nre user remove --username <name>\n\n";
    std::cout << "  nre audit list [--limit <n>]\n";
    std::cout << "  nre audit export --output <path>\n\n";
    std::cout << "  nre test run --profile-name <name> --iface <name> --duration <seconds>\n";
    std::cout << "  nre test list\n\n";
    std::cout << "  nre status\n\n";
}

ExperimentProfile parseFlagsIntoProfile(int argc, char *argv[], int startIndex, std::string &interfaceOut) {
    ExperimentProfile profile;
    for (int i = startIndex; i < argc - 1; i += 2) {
        std::string flag = argv[i];
        std::string value = argv[i + 1];
        if (flag == "--iface") interfaceOut = value;
        else if (flag == "--target-ip") profile.targetIp = value;
        else if (flag == "--target-mac") profile.targetMac = value;
        else if (flag == "--latency") profile.latencyMs = std::atoi(value.c_str());
        else if (flag == "--jitter") profile.jitterMs = std::atoi(value.c_str());
        else if (flag == "--loss") profile.packetLossPc = std::atof(value.c_str());
        else if (flag == "--download") profile.downloadKbit = std::atoi(value.c_str());
        else if (flag == "--upload") profile.uploadKbit = std::atoi(value.c_str());
        else if (flag == "--duration") profile.durationSeconds = std::atoi(value.c_str());
        else if (flag == "--name") profile.name = value;
        else if (flag == "--profile-name") profile.name = value;
    }
    return profile;
}

std::string getFlag(int argc, char *argv[], const std::string &flag) {
    for (int i = 2; i < argc - 1; i += 2) {
        if (argv[i] == flag) return argv[i + 1];
    }
    return "";
}

bool hasFlag(int argc, char *argv[], const std::string &flag) {
    for (int i = 2; i < argc; ++i) {
        if (argv[i] == flag) return true;
    }
    return false;
}

void runExperiment(const ExperimentProfile &profile, const std::string &interfaceName) {
    Logger logger("nre.log");
    TcControl tcControl(logger);
    TargetSelector targetSelector(logger);
    NftControl nftControl(logger);
    ResourceGuard guard(logger);

    std::string experimentName = profile.name.empty() ? "Manual Experiment" : profile.name;

    Experiment experiment(experimentName, interfaceName,
                           profile.targetIp, profile.targetMac,
                           profile.latencyMs, profile.jitterMs, profile.packetLossPc,
                           profile.downloadKbit, profile.uploadKbit, profile.durationSeconds,
                           logger, tcControl, targetSelector, &guard);

    experiment.printSummary();
    std::string startTime = nowTimestamp();
    MetricsResult metrics = experiment.run();
    std::string endTime = nowTimestamp();

    HistoryManager historyManager(logger);
    ExperimentRecord record;
    record.name = experimentName;
    record.type = "impairment";
    record.target = profile.targetIp.empty()
        ? (profile.targetMac.empty() ? "(whole interface)" : profile.targetMac)
        : profile.targetIp;
    record.startTime = startTime;
    record.endTime = endTime;
    record.durationSeconds = profile.durationSeconds;
    record.status = experiment.wasSuccessful() ? "completed" : "aborted";
    record.avgLatencyMs = metrics.avgLatencyMs;
    record.packetLossPc = metrics.packetLossPc;
    record.jitterMs = metrics.jitterMs;
    record.avgBandwidthMbps = metrics.avgBandwidthMbps;
    record.healthStatus = NetworkMonitor::healthStatusToString(NetworkMonitor::computeHealthStatus(metrics));
    int historyId = historyManager.addRecord(record);
    if (historyId > 0) std::cout << "Saved to history as experiment #" << historyId << std::endl;

    PolicyManager policyManager(logger);
    std::vector<Policy> policies = policyManager.loadAllPolicies();
    if (!policies.empty()) {
        std::cout << "\nPolicy Engine: checking " << policies.size() << " saved polic"
                   << (policies.size() == 1 ? "y" : "ies") << "..." << std::endl;
        PolicyEngine policyEngine(logger, nftControl, tcControl);
        int triggered = policyEngine.evaluateAndRun(policies, metrics, interfaceName);
        if (triggered == 0) std::cout << "No policies were triggered." << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) { printUsage(); return 1; }

    std::string command = argv[1];

    if (command == "start") {
        std::string interfaceName = "eth0";
        ExperimentProfile profile = parseFlagsIntoProfile(argc, argv, 2, interfaceName);
        runExperiment(profile, interfaceName);
        return 0;
    }

    if (command == "profile") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        ProfileManager pm(logger);

        if (sub == "save") {
            std::string iface;
            ExperimentProfile profile = parseFlagsIntoProfile(argc, argv, 3, iface);
            if (profile.name.empty()) { std::cout << "Provide --profile-name.\n"; return 1; }
            pm.saveProfile(profile);
            std::cout << "Profile saved: " << profile.name << std::endl;
            return 0;
        }
        if (sub == "start") {
            std::string iface = "eth0";
            std::string name = getFlag(argc, argv, "--profile-name");
            std::string fIface = getFlag(argc, argv, "--iface");
            if (!fIface.empty()) iface = fIface;
            if (name.empty()) { std::cout << "Provide --profile-name.\n"; return 1; }
            bool found = false;
            ExperimentProfile profile = pm.loadProfile(name, found);
            if (!found) { std::cout << "Profile '" << name << "' not found.\n"; return 1; }
            runExperiment(profile, iface);
            return 0;
        }
        if (sub == "list") {
            std::vector<std::string> names = pm.listProfiles();
            if (names.empty()) std::cout << "No saved profiles.\n";
            else { std::cout << "Profiles:\n"; for (auto &n : names) std::cout << "  - " << n << "\n"; }
            return 0;
        }
        std::cout << "Unknown profile command: " << sub << std::endl;
        return 1;
    }

    if (command == "dns-failure" || command == "disconnect") {
        std::string targetIp = getFlag(argc, argv, "--target-ip");
        std::string targetMac = getFlag(argc, argv, "--target-mac");
        std::string name = getFlag(argc, argv, "--name");
        int duration = std::atoi(getFlag(argc, argv, "--duration").c_str());
        if (duration <= 0) duration = 30;
        if (targetIp.empty() && targetMac.empty()) { std::cout << "Provide --target-ip or --target-mac.\n"; return 1; }

        FailureType type = (command == "dns-failure") ? FailureType::DNS_FAILURE : FailureType::CONNECTION_INTERRUPTION;
        if (name.empty()) name = (command == "dns-failure") ? "DNS Failure" : "Connection Interruption";

        Logger logger("nre.log");
        NftControl nft(logger);
        TargetSelector ts(logger);
        ResourceGuard guard(logger);

        FailureExperiment exp(name, type, targetIp, targetMac, duration, logger, nft, ts, &guard);
        exp.printSummary();
        std::string startTime = nowTimestamp();
        exp.run();
        std::string endTime = nowTimestamp();

        HistoryManager hm(logger);
        ExperimentRecord record;
        record.name = name;
        record.type = (command == "dns-failure") ? "dns_failure" : "disconnect";
        record.target = targetIp.empty() ? targetMac : targetIp;
        record.startTime = startTime; record.endTime = endTime;
        record.durationSeconds = duration;
        record.status = exp.wasSuccessful() ? "completed" : "aborted";
        record.healthStatus = "N/A";
        int hid = hm.addRecord(record);
        if (hid > 0) std::cout << "Saved to history as experiment #" << hid << std::endl;
        return 0;
    }

    if (command == "policy") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        PolicyManager pm(logger);

        if (sub == "save") {
            Policy policy;
            Condition slots[MAX_CONDITIONS];
            for (int i = 3; i < argc - 1; i += 2) {
                std::string flag = argv[i];
                std::string value = argv[i + 1];
                if (flag == "--policy-name") policy.name = value;
                else if (flag == "--logic") policy.logic = value;
                else if (flag == "--action") policy.action = value;
                else if (flag == "--action-param") policy.actionParam = value;
                else if (flag.rfind("--cond", 0) == 0) {
                    int index = flag[6] - '1';
                    if (index < 0 || index >= MAX_CONDITIONS) continue;
                    if (flag.find("-metric") != std::string::npos) slots[index].metric = value;
                    else if (flag.find("-op") != std::string::npos) slots[index].op = value;
                    else if (flag.find("-value") != std::string::npos) slots[index].value = std::atof(value.c_str());
                }
            }
            for (int i = 0; i < MAX_CONDITIONS; ++i)
                if (!slots[i].isEmpty()) policy.conditions.push_back(slots[i]);
            if (policy.name.empty()) { std::cout << "Provide --policy-name.\n"; return 1; }
            if (policy.conditions.empty()) { std::cout << "Provide at least one condition.\n"; return 1; }
            if (policy.action.empty()) { std::cout << "Provide --action.\n"; return 1; }
            pm.savePolicy(policy);
            std::cout << "Policy saved: " << policy.name << std::endl;
            return 0;
        }
        if (sub == "list") {
            std::vector<std::string> names = pm.listPolicyNames();
            if (names.empty()) std::cout << "No saved policies.\n";
            else { std::cout << "Policies:\n"; for (auto &n : names) std::cout << "  - " << n << "\n"; }
            return 0;
        }
        if (sub == "test") {
            std::string pname = getFlag(argc, argv, "--policy-name");
            MetricsResult fake;
            for (int i = 3; i < argc - 1; i += 2) {
                std::string f = argv[i], v = argv[i+1];
                if (f == "--latency") fake.avgLatencyMs = std::atof(v.c_str());
                else if (f == "--loss") fake.packetLossPc = std::atof(v.c_str());
                else if (f == "--jitter") fake.jitterMs = std::atof(v.c_str());
                else if (f == "--bandwidth") fake.avgBandwidthMbps = std::atof(v.c_str());
            }
            if (pname.empty()) { std::cout << "Provide --policy-name.\n"; return 1; }
            bool found = false;
            Policy p = pm.loadPolicy(pname, found);
            if (!found) { std::cout << "Policy '" << pname << "' not found.\n"; return 1; }
            bool matched = PolicyEngine::evaluatePolicy(p, fake);
            std::cout << "Policy '" << pname << "' -> " << (matched ? "TRIGGERED" : "not triggered") << std::endl;
            return 0;
        }
        std::cout << "Unknown policy command: " << sub << std::endl;
        return 1;
    }

    if (command == "history") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        HistoryManager hm(logger);
        if (sub == "list") {
            int limit = 0;
            std::string l = getFlag(argc, argv, "--limit");
            if (!l.empty()) limit = std::atoi(l.c_str());
            hm.printHistory(limit);
            return 0;
        }
        if (sub == "export") {
            std::string fmt = getFlag(argc, argv, "--format");
            std::string out = getFlag(argc, argv, "--output");
            if (out.empty()) { std::cout << "Provide --output.\n"; return 1; }
            bool ok = false;
            if (fmt == "json") ok = hm.exportJson(out);
            else if (fmt == "csv") ok = hm.exportCsv(out);
            else { std::cout << "Unknown format. Use json or csv.\n"; return 1; }
            if (ok) std::cout << "Exported to " << out << std::endl;
            return ok ? 0 : 1;
        }
        std::cout << "Unknown history command: " << sub << std::endl;
        return 1;
    }

    if (command == "chain") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        ChainManager cm(logger);

        if (sub == "save") {
            Chain chain;
            chain.name = getFlag(argc, argv, "--chain-name");
            if (chain.name.empty()) { std::cout << "Provide --chain-name.\n"; return 1; }
            ProfileManager pm(logger);
            for (int i = 3; i < argc - 1; i += 2) {
                std::string flag = argv[i], value = argv[i+1];
                if (flag.rfind("--profile", 0) == 0) {
                    bool found = false;
                    ExperimentProfile p = pm.loadProfile(value, found);
                    if (found) chain.steps.push_back(p);
                    else std::cout << "Profile '" << value << "' not found, skipping.\n";
                }
            }
            if (chain.steps.empty()) { std::cout << "Provide at least one --profile <name>.\n"; return 1; }
            cm.saveChain(chain);
            std::cout << "Chain saved: " << chain.name << " (" << chain.steps.size() << " steps)\n";
            return 0;
        }
        if (sub == "list") {
            std::vector<std::string> names = cm.listChainNames();
            if (names.empty()) std::cout << "No saved chains.\n";
            else { std::cout << "Chains:\n"; for (auto &n : names) std::cout << "  - " << n << "\n"; }
            return 0;
        }
        if (sub == "start") {
            std::string cname = getFlag(argc, argv, "--chain-name");
            std::string iface = getFlag(argc, argv, "--iface");
            if (iface.empty()) iface = "eth0";
            if (cname.empty()) { std::cout << "Provide --chain-name.\n"; return 1; }
            bool found = false;
            Chain chain = cm.loadChain(cname, found);
            if (!found) { std::cout << "Chain '" << cname << "' not found.\n"; return 1; }
            TcControl tc(logger);
            TargetSelector ts(logger);
            ResourceGuard guard(logger);
            cm.executeChain(chain, iface, tc, ts, &guard);
            return 0;
        }
        std::cout << "Unknown chain command: " << sub << std::endl;
        return 1;
    }

    if (command == "preset") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        PresetManager pm;

        if (sub == "list") {
            std::vector<std::string> names = pm.listPresetNames();
            std::cout << "Built-in presets:\n";
            for (auto &n : names) std::cout << "  - " << n << "\n";
            return 0;
        }
        if (sub == "info") {
            std::string pname = getFlag(argc, argv, "--preset-name");
            if (pname.empty()) { std::cout << "Provide --preset-name.\n"; return 1; }
            bool found = false;
            PresetScenario p = pm.getPreset(pname, found);
            if (!found) { std::cout << "Preset '" << pname << "' not found.\n"; return 1; }
            std::cout << "Preset: " << p.name << "\n";
            std::cout << "Description: " << p.description << "\n";
            std::cout << "Latency: " << p.profile.latencyMs << " ms\n";
            std::cout << "Jitter: " << p.profile.jitterMs << " ms\n";
            std::cout << "Packet loss: " << p.profile.packetLossPc << " %\n";
            std::cout << "Download: " << (p.profile.downloadKbit > 0 ? std::to_string(p.profile.downloadKbit) + " kbit/s" : "unlimited") << "\n";
            std::cout << "Upload: " << (p.profile.uploadKbit > 0 ? std::to_string(p.profile.uploadKbit) + " kbit/s" : "unlimited") << "\n";
            std::cout << "Duration: " << p.profile.durationSeconds << " s\n";
            return 0;
        }
        if (sub == "export") {
            std::string pname = getFlag(argc, argv, "--preset-name");
            std::string out = getFlag(argc, argv, "--output");
            if (pname.empty() || out.empty()) { std::cout << "Provide --preset-name and --output.\n"; return 1; }
            if (pm.exportPreset(pname, out)) std::cout << "Exported preset '" << pname << "' to " << out << std::endl;
            else { std::cout << "Preset '" << pname << "' not found.\n"; return 1; }
            return 0;
        }
        if (sub == "export-all") {
            std::string out = getFlag(argc, argv, "--output");
            if (out.empty()) { std::cout << "Provide --output.\n"; return 1; }
            if (pm.exportAllPresets(out)) std::cout << "All presets exported to " << out << std::endl;
            else { std::cout << "Export failed.\n"; return 1; }
            return 0;
        }
        if (sub == "import") {
            std::string inp = getFlag(argc, argv, "--input");
            if (inp.empty()) { std::cout << "Provide --input.\n"; return 1; }
            if (pm.importPreset(inp)) std::cout << "Preset imported from " << inp << std::endl;
            else { std::cout << "Import failed.\n"; return 1; }
            return 0;
        }
        std::cout << "Unknown preset command: " << sub << std::endl;
        return 1;
    }

    if (command == "schedule") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        ExperimentScheduler sched(logger);

        if (sub == "add") {
            ScheduledExperiment se;
            se.profileName = getFlag(argc, argv, "--profile-name");
            se.interfaceName = getFlag(argc, argv, "--iface");
            se.scheduledTime = getFlag(argc, argv, "--time");
            se.repeatDaily = hasFlag(argc, argv, "--repeat");
            if (se.profileName.empty()) { std::cout << "Provide --profile-name.\n"; return 1; }
            if (se.interfaceName.empty()) se.interfaceName = "eth0";
            sched.addSchedule(se);
            return 0;
        }
        if (sub == "list") {
            std::vector<ScheduledExperiment> list = sched.listSchedules();
            if (list.empty()) std::cout << "No scheduled experiments.\n";
            else {
                std::cout << "Scheduled experiments:\n";
                for (auto &s : list)
                    std::cout << "  #" << s.id << " profile=" << s.profileName
                              << " time=" << s.scheduledTime
                              << (s.repeatDaily ? " (daily)" : "") << "\n";
            }
            return 0;
        }
        if (sub == "remove") {
            std::string id = getFlag(argc, argv, "--id");
            if (id.empty()) { std::cout << "Provide --id.\n"; return 1; }
            sched.removeSchedule(std::atoi(id.c_str()));
            return 0;
        }
        std::cout << "Unknown schedule command: " << sub << std::endl;
        return 1;
    }

    if (command == "daemon") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        static Logger daemonLogger("nre.log");
        static NreDaemon daemon(daemonLogger);

        if (sub == "start") {
            std::string target = getFlag(argc, argv, "--target");
            std::string iface = getFlag(argc, argv, "--iface");
            std::string interval = getFlag(argc, argv, "--interval");
            if (!target.empty()) daemon.setMonitorTarget(target);
            if (!iface.empty()) daemon.setMonitorInterface(iface);
            if (!interval.empty()) daemon.setMonitorInterval(std::atoi(interval.c_str()));
            daemon.start();
            std::cout << "Daemon running. Press Ctrl+C to stop.\n";
            while (daemon.isRunning()) { sleep(1); }
            return 0;
        }
        if (sub == "stop") {
            daemon.stop();
            return 0;
        }
        if (sub == "status") {
            std::cout << "Daemon: " << (daemon.isRunning() ? "running" : "stopped") << std::endl;
            return 0;
        }
        std::cout << "Unknown daemon command: " << sub << std::endl;
        return 1;
    }

    if (command == "api") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        static Logger apiLogger("nre.log");
        static RestApi api(apiLogger);

        if (sub == "start") {
            std::string port = getFlag(argc, argv, "--port");
            if (!port.empty()) api = RestApi(apiLogger, std::atoi(port.c_str()));
            api.start();
            std::cout << "API server running. Press Ctrl+C to stop.\n";
            while (api.isRunning()) { sleep(1); }
            return 0;
        }
        if (sub == "stop") {
            api.stop();
            return 0;
        }
        std::cout << "Unknown api command: " << sub << std::endl;
        return 1;
    }

    if (command == "user") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        PermissionManager pm(logger);

        if (sub == "add") {
            std::string uname = getFlag(argc, argv, "--username");
            std::string pass = getFlag(argc, argv, "--password");
            std::string lvl = getFlag(argc, argv, "--level");
            if (uname.empty() || pass.empty()) { std::cout << "Provide --username and --password.\n"; return 1; }
            PermissionLevel level = PermissionLevel::VIEWER;
            if (lvl == "admin") level = PermissionLevel::ADMIN;
            else if (lvl == "operator") level = PermissionLevel::OPERATOR;
            pm.addUser(uname, pass, level);
            std::cout << "User added: " << uname << std::endl;
            return 0;
        }
        if (sub == "list") {
            std::vector<User> users = pm.listUsers();
            std::cout << "Users:\n";
            for (auto &u : users)
                std::cout << "  " << u.username << " (level: "
                          << (u.level == PermissionLevel::ADMIN ? "admin" :
                              u.level == PermissionLevel::OPERATOR ? "operator" : "viewer") << ")\n";
            return 0;
        }
        if (sub == "remove") {
            std::string uname = getFlag(argc, argv, "--username");
            if (uname.empty()) { std::cout << "Provide --username.\n"; return 1; }
            pm.removeUser(uname);
            std::cout << "User removed: " << uname << std::endl;
            return 0;
        }
        std::cout << "Unknown user command: " << sub << std::endl;
        return 1;
    }

    if (command == "audit") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        AuditLog al(logger);

        if (sub == "list") {
            int limit = 0;
            std::string l = getFlag(argc, argv, "--limit");
            if (!l.empty()) limit = std::atoi(l.c_str());
            al.printLog(limit);
            return 0;
        }
        if (sub == "export") {
            std::string out = getFlag(argc, argv, "--output");
            if (out.empty()) { std::cout << "Provide --output.\n"; return 1; }
            al.exportJson(out);
            std::cout << "Exported to " << out << std::endl;
            return 0;
        }
        std::cout << "Unknown audit command: " << sub << std::endl;
        return 1;
    }

    if (command == "test") {
        if (argc < 3) { printUsage(); return 1; }
        std::string sub = argv[2];
        Logger logger("nre.log");
        TcControl tc(logger);
        NftControl nft(logger);
        TargetSelector ts(logger);
        ResourceGuard guard(logger);
        TestAutomation ta(logger, tc, nft, ts, &guard);

        if (sub == "run") {
            std::string pname = getFlag(argc, argv, "--profile-name");
            std::string iface = getFlag(argc, argv, "--iface");
            std::string dur = getFlag(argc, argv, "--duration");
            if (pname.empty()) { std::cout << "Provide --profile-name.\n"; return 1; }
            if (iface.empty()) iface = "eth0";
            int duration = dur.empty() ? 60 : std::atoi(dur.c_str());

            ProfileManager pm(logger);
            bool found = false;
            ExperimentProfile profile = pm.loadProfile(pname, found);
            if (!found) { std::cout << "Profile '" << pname << "' not found.\n"; return 1; }

            TestScenario scenario = ta.createStandardTest(pname, iface, duration);
            bool ok = ta.runScenario(scenario, iface);
            return ok ? 0 : 1;
        }
        if (sub == "list") {
            std::vector<std::string> scenarios = ta.listScenarios();
            std::cout << "Available test scenarios:\n";
            for (auto &s : scenarios) std::cout << "  - " << s << "\n";
            return 0;
        }
        std::cout << "Unknown test command: " << sub << std::endl;
        return 1;
    }

    if (command == "status") {
        Logger logger("nre.log");
        std::cout << "=== NREPlatform Status ===" << std::endl;

        ProfileManager pm(logger);
        std::vector<std::string> profiles = pm.listProfiles();
        std::cout << "Profiles: " << profiles.size() << std::endl;

        PolicyManager polm(logger);
        std::vector<std::string> policies = polm.listPolicyNames();
        std::cout << "Policies: " << policies.size() << std::endl;

        HistoryManager hm(logger);
        std::vector<ExperimentRecord> history = hm.loadAll();
        std::cout << "Experiments in history: " << history.size() << std::endl;

        ChainManager cm(logger);
        std::vector<std::string> chains = cm.listChainNames();
        std::cout << "Chains: " << chains.size() << std::endl;

        ResourceGuard guard(logger);
        std::cout << "Running experiments: " << guard.countRunningExperiments() << "/3" << std::endl;

        PresetManager presetMgr;
        std::cout << "Built-in presets: " << presetMgr.listPresetNames().size() << std::endl;

        return 0;
    }

    std::cout << "Unknown command: " << command << std::endl;
    printUsage();
    return 1;
}
