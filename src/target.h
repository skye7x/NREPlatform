#ifndef TARGET_H
#define TARGET_H

#include <string>
#include "logger.h"

struct TargetInfo {
    std::string ipAddress;
    std::string macAddress;
    std::string ipv6Address;
    std::string subnet;       
    int tcpPort = 0;          
    int udpPort = 0;          
    bool isIPv6 = false;
    bool isSubnet = false;
};

class TargetSelector {
public:
    TargetSelector(Logger &logger);

    std::string resolveMacToIp(const std::string &macAddress);

    std::string resolveMacToIpv6(const std::string &macAddress);

    TargetInfo parseTarget(const std::string &targetSpec);

    std::string buildTcFilterMatch(const TargetInfo &target, const std::string &direction);

    std::string buildNftMatch(const TargetInfo &target);

private:
    Logger &log;
    std::string toLower(const std::string &text);

    bool isIPv4(const std::string &address);

    bool isIPv6(const std::string &address);

    bool isSubnet(const std::string &address);
};

#endif
