#ifndef CHAIN_H
#define CHAIN_H

#include <string>
#include <vector>
#include "logger.h"
#include "profile.h"
#include "tc_control.h"
#include "nft_control.h"
#include "target.h"
#include "resource_guard.h"

const int MAX_CHAIN_STEPS = 10;

struct Chain {
    std::string name;
    std::vector<ExperimentProfile> steps;
};

class ChainManager {
public:
    ChainManager(Logger &logger, const std::string &folder = "chains");

    bool saveChain(const Chain &chain);
    Chain loadChain(const std::string &name, bool &found);
    std::vector<std::string> listChainNames();

    bool executeChain(const Chain &chain,
                      const std::string &interfaceName,
                      TcControl &tcControl,
                      TargetSelector &targetSelector,
                      ResourceGuard *guard = nullptr);

private:
    Logger &log;
    std::string folder;
};

#endif
