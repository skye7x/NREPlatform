#include "failure_experiment.h"
#include <iostream>
#include <thread>
#include <chrono>

FailureExperiment::FailureExperiment(const std::string &name,
                                      FailureType type,
                                      const std::string &targetIp,
                                      const std::string &targetMac,
                                      int durationSeconds,
                                      Logger &logger,
                                      NftControl &nftControl,
                                      TargetSelector &selector,
                                      ResourceGuard *resourceGuard)
    : name(name),
      type(type),
      targetIp(targetIp),
      targetMac(targetMac),
      durationSeconds(durationSeconds),
      log(logger),
      nft(nftControl),
      targetSelector(selector),
      guard(resourceGuard) {
}

void FailureExperiment::printSummary() const {
    std::cout << "Experiment: " << name << std::endl;
    std::cout << "  Type       : "
              << (type == FailureType::DNS_FAILURE ? "DNS Failure" : "Connection Interruption")
              << std::endl;
    std::cout << "  Target IP  : " << (targetIp.empty() ? "(not set)" : targetIp) << std::endl;
    std::cout << "  Target MAC : " << (targetMac.empty() ? "(none)" : targetMac) << std::endl;
    std::cout << "  Duration   : " << durationSeconds << " seconds" << std::endl;
}

void FailureExperiment::run() {
    log.log("Experiment started: " + name);

    if (guard) {
        if (!guard->checkDuration(durationSeconds)) {
            log.log("Experiment aborted: duration exceeds resource limit.");
            std::cout << "REJECTED: duration " << durationSeconds << "s exceeds maximum allowed.\n";
            return;
        }
        if (!guard->tryAcquireSlot(name, resourceSlotId)) {
            log.log("Experiment aborted: too many concurrent experiments.");
            std::cout << "REJECTED: maximum concurrent experiments already reached.\n";
            return;
        }
    }

    std::string resolvedIp = targetIp;
    if (resolvedIp.empty() && !targetMac.empty()) {
        resolvedIp = targetSelector.resolveMacToIp(targetMac);
    }

    if (resolvedIp.empty()) {
        log.log("Experiment aborted: no target IP could be determined.");
        return;
    }

    if (!nft.setupTable()) {
        log.log("Experiment aborted: could not set up nftables table.");
        return;
    }

    bool applied = false;
    if (type == FailureType::DNS_FAILURE) {
        applied = nft.blockDns(resolvedIp);
    } else {
        applied = nft.blockConnection(resolvedIp);
    }

    if (!applied) {
        log.log("Experiment aborted: could not apply block rules.");
        nft.teardownTable(); 
        return;
    }

    std::cout << "Experiment running for " << durationSeconds << " seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));

    nft.teardownTable();

    log.log("Experiment finished: " + name);

    if (guard) {
        guard->releaseSlot(resourceSlotId);
    }

    success = true;
}
