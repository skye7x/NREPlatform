#include "daemon.h"
#include <iostream>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctime>

static NreDaemon *g_daemonInstance = nullptr;

static void signalHandler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        std::cout << "\n[DAEMON] Received signal " << signum << ", shutting down..." << std::endl;
        if (g_daemonInstance) {
            g_daemonInstance->stop();
        }
    }
}

NreDaemon::NreDaemon(Logger &logger)
    : log(logger),
      tcControl(logger),
      nftControl(logger),
      targetSelector(logger),
      policyEngine(logger, nftControl, tcControl),
      resourceGuard(logger),
      scheduler(logger),
      running(false),
      monitorInterface("eth0"),
      monitorIntervalSeconds(60) {
    g_daemonInstance = this;
}

NreDaemon::~NreDaemon() {
    stop();
    g_daemonInstance = nullptr;
}

void NreDaemon::setMonitorTarget(const std::string &target) {
    monitorTarget = target;
}

void NreDaemon::setMonitorInterface(const std::string &interface) {
    monitorInterface = interface;
}

void NreDaemon::setMonitorInterval(int seconds) {
    monitorIntervalSeconds = seconds;
}

bool NreDaemon::start() {
    if (running) {
        std::cout << "Daemon is already running." << std::endl;
        return false;
    }

    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    std::string pidFile = "/var/run/nre.pid";
    writePidFile(pidFile);

    running = true;
    daemonThread = std::thread(&NreDaemon::daemonLoop, this);

    log.log("NREPlatform daemon started (PID " + std::to_string(getpid()) + ")");
    std::cout << "NREPlatform daemon started (PID " << getpid() << ")" << std::endl;

    scheduler.start();

    return true;
}

bool NreDaemon::stop() {
    if (!running) return true;

    running = false;

    scheduler.stop();

    if (daemonThread.joinable()) {
        daemonThread.join();
    }

    removePidFile("/var/run/nre.pid");

    log.log("NREPlatform daemon stopped.");
    std::cout << "NREPlatform daemon stopped." << std::endl;

    return true;
}

bool NreDaemon::isRunning() const {
    return running;
}

void NreDaemon::daemonLoop() {
    log.log("Daemon monitoring loop started (interval: " +
            std::to_string(monitorIntervalSeconds) + "s)");
    std::cout << "[DAEMON] Monitoring loop started." << std::endl;

    while (running) {
        monitorAndAct();
        std::this_thread::sleep_for(std::chrono::seconds(monitorIntervalSeconds));
    }
}

void NreDaemon::monitorAndAct() {
    if (monitorTarget.empty()) {
        
        return;
    }

    NetworkMonitor monitor(log, monitorTarget, monitorInterface);

    MetricsResult metrics = monitor.collectFor(5);

    HealthStatus status = NetworkMonitor::computeHealthStatus(metrics);
    std::string statusStr = NetworkMonitor::healthStatusToString(status);

    log.log("[DAEMON] Health check: " + statusStr +
            " (latency=" + std::to_string((int)metrics.avgLatencyMs) + "ms, " +
            "loss=" + std::to_string((int)metrics.packetLossPc) + "%)");

    if (status != HealthStatus::HEALTHY) {
        log.log("[DAEMON] Network degraded, evaluating policies...");
        std::cout << "[DAEMON] Network " << statusStr << " - evaluating policies." << std::endl;

        PolicyManager policyManager(log);
        std::vector<Policy> policies = policyManager.loadAllPolicies();

        if (!policies.empty()) {
            int triggered = policyEngine.evaluateAndRun(policies, metrics, monitorInterface);
            if (triggered > 0) {
                log.log("[DAEMON] " + std::to_string(triggered) + " policy(ies) triggered.");
            }
        }
    }
}

bool writePidFile(const std::string &path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << getpid() << "\n";
    file.close();
    return true;
}

bool readPidFile(const std::string &path, pid_t &pid) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    file >> pid;
    return true;
}

bool removePidFile(const std::string &path) {
    return remove(path.c_str()) == 0;
}
