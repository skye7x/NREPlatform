#ifndef TC_CONTROL_H
#define TC_CONTROL_H

#include <string>
#include "logger.h"

class TcControl {
public:
    TcControl(Logger &logger);

    bool applyNetem(const std::string &interfaceName, int latencyMs, double packetLossPc);

    bool removeNetem(const std::string &interfaceName);

    bool isActive() const;

    bool applyShapedNetem(const std::string &interfaceName,
                           const std::string &targetIp,
                           const std::string &matchField,
                           int latencyMs,
                           int jitterMs,
                           double packetLossPc,
                           int bandwidthKbit);

    bool removeShapedNetem(const std::string &interfaceName);

    bool setupIfbRedirect(const std::string &lanInterface, const std::string &ifbInterface);
    bool teardownIfbRedirect(const std::string &lanInterface, const std::string &ifbInterface);

private:
    Logger &log;          
    bool active;          

    bool runCommand(const std::string &command);
};

#endif
