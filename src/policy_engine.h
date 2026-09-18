#ifndef POLICY_ENGINE_H
#define POLICY_ENGINE_H

#include <vector>
#include <functional>
#include "policy.h"
#include "monitor.h"
#include "logger.h"
#include "nft_control.h"
#include "tc_control.h"
#include "profile.h"

class PolicyEngine {
public:
    PolicyEngine(Logger &logger, NftControl &nftControl, TcControl &tcControl);

    static bool evaluatePolicy(const Policy &policy, const MetricsResult &metrics);

    int evaluateAndRun(const std::vector<Policy> &policies,
                        const MetricsResult &metrics,
                        const std::string &defaultInterface);

    using ExperimentStartCallback = std::function<bool(const ExperimentProfile &, const std::string &)>;
    using ExperimentStopCallback = std::function<bool()>;
    void setExperimentCallbacks(ExperimentStartCallback startCb, ExperimentStopCallback stopCb);

private:
    Logger &log;
    NftControl &nft;
    TcControl &tc;

    ExperimentStartCallback startExperimentCb;
    ExperimentStopCallback stopExperimentCb;

    void executeAction(const Policy &policy, const std::string &defaultInterface);
};

#endif
