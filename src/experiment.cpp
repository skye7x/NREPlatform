#include "experiment.h"
#include <iostream>
#include <thread>
#include <chrono>

Experiment::Experiment(const std::string &name,
                        const std::string &lanInterface,
                        const std::string &targetIp,
                        const std::string &targetMac,
                        int latencyMs,
                        int jitterMs,
                        double packetLossPc,
                        int downloadKbit,
                        int uploadKbit,
                        int durationSeconds,
                        Logger &logger,
                        TcControl &tcControl,
                        TargetSelector &selector,
                        ResourceGuard *resourceGuard)
    : name(name),
      lanInterface(lanInterface),
      targetIp(targetIp),
      targetMac(targetMac),
      latencyMs(latencyMs),
      jitterMs(jitterMs),
      packetLossPc(packetLossPc),
      downloadKbit(downloadKbit),
      uploadKbit(uploadKbit),
      durationSeconds(durationSeconds),
      log(logger),
      tc(tcControl),
      targetSelector(selector),
      guard(resourceGuard) {
}

void Experiment::printSummary() const {
    std::cout << "Experiment: " << name << std::endl;
    std::cout << "  Interface   : " << lanInterface << std::endl;
    std::cout << "  Target IP   : " << (targetIp.empty() ? "(whole interface)" : targetIp) << std::endl;
    std::cout << "  Target MAC  : " << (targetMac.empty() ? "(none)" : targetMac) << std::endl;
    std::cout << "  Latency     : " << latencyMs << " ms" << std::endl;
    std::cout << "  Jitter      : " << jitterMs << " ms" << std::endl;
    std::cout << "  Packet loss : " << packetLossPc << " %" << std::endl;
    std::cout << "  Download    : " << (downloadKbit > 0 ? std::to_string(downloadKbit) + " kbit/s" : "unlimited") << std::endl;
    std::cout << "  Upload      : " << (uploadKbit > 0 ? std::to_string(uploadKbit) + " kbit/s" : "unlimited") << std::endl;
    std::cout << "  Duration    : " << durationSeconds << " seconds" << std::endl;
}

MetricsResult Experiment::run() {
    log.log("Experiment started: " + name + " on " + lanInterface);

    MetricsResult emptyResult; 

    if (guard) {
        if (!guard->checkDuration(durationSeconds)) {
            log.log("Experiment aborted: duration exceeds resource limit.");
            std::cout << "REJECTED: duration " << durationSeconds << "s exceeds maximum allowed.\n";
            return emptyResult;
        }
        if (!guard->tryAcquireSlot(name, resourceSlotId)) {
            log.log("Experiment aborted: too many concurrent experiments.");
            std::cout << "REJECTED: maximum concurrent experiments already reached.\n";
            return emptyResult;
        }
    }

    std::string resolvedIp = targetIp;
    if (resolvedIp.empty() && !targetMac.empty()) {
        resolvedIp = targetSelector.resolveMacToIp(targetMac);
        if (resolvedIp.empty()) {
            log.log("Experiment aborted: could not resolve target MAC to an IP.");
            return emptyResult;
        }
    }

    bool downloadApplied = tc.applyShapedNetem(lanInterface,
                                                resolvedIp,
                                                "dst",
                                                latencyMs,
                                                jitterMs,
                                                packetLossPc,
                                                downloadKbit);

    if (!downloadApplied) {
        log.log("Experiment aborted: could not apply download shaping.");
        return emptyResult;
    }

    if (uploadKbit > 0) {
        bool ifbReady = tc.setupIfbRedirect(lanInterface, ifbInterface);

        if (ifbReady) {
            bool uploadApplied = tc.applyShapedNetem(ifbInterface,
                                                      resolvedIp,
                                                      "src",
                                                      latencyMs,
                                                      jitterMs,
                                                      packetLossPc,
                                                      uploadKbit);
            uploadShapingActive = uploadApplied;

            if (!uploadApplied) {
                log.log("WARNING: upload shaping failed, continuing with download shaping only.");
            }
        } else {
            log.log("WARNING: could not set up IFB redirect, skipping upload shaping.");
        }
    }

    NetworkMonitor monitor(log, resolvedIp, lanInterface);
    MetricsResult metrics = monitor.collectFor(durationSeconds);
    NetworkMonitor::printResult(metrics);

    HealthStatus status = NetworkMonitor::computeHealthStatus(metrics);
    log.log("Network health during experiment: " + NetworkMonitor::healthStatusToString(status));

    tc.removeShapedNetem(lanInterface);

    if (uploadShapingActive) {
        tc.removeShapedNetem(ifbInterface);
        tc.teardownIfbRedirect(lanInterface, ifbInterface);
    }

    log.log("Experiment finished: " + name);

    if (guard) {
        guard->releaseSlot(resourceSlotId);
    }

    success = true;
    return metrics;
}
