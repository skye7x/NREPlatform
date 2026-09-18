#ifndef NFT_CONTROL_H
#define NFT_CONTROL_H

#include <string>
#include "logger.h"

class NftControl {
public:
    NftControl(Logger &logger);

    bool setupTable();

    bool teardownTable();

    bool blockDns(const std::string &targetIp);

    bool blockConnection(const std::string &targetIp);

private:
    Logger &log;
    const std::string tableName = "nre_platform";
    const std::string chainName = "block";

    bool runCommand(const std::string &command);
};

#endif
