#include "nft_control.h"
#include <cstdlib>

NftControl::NftControl(Logger &logger) : log(logger) {
}

bool NftControl::runCommand(const std::string &command) {
    log.log("RUN: " + command);

    int result = std::system(command.c_str());

    if (result != 0) {
        log.log("ERROR: command failed (exit code " + std::to_string(result) + ")");
        return false;
    }

    return true;
}

bool NftControl::setupTable() {
    
    std::system(("sudo nft delete table inet " + tableName).c_str());

    if (!runCommand("sudo nft add table inet " + tableName)) return false;

    std::string chainCmd = "sudo nft add chain inet " + tableName + " " + chainName +
        " { type filter hook forward priority -1 \\; }";

    if (!runCommand(chainCmd)) return false;

    log.log("nftables table '" + tableName + "' ready.");
    return true;
}

bool NftControl::teardownTable() {
    bool success = runCommand("sudo nft delete table inet " + tableName);

    if (success) {
        log.log("nftables table '" + tableName + "' removed (configuration restored).");
    }

    return success;
}

bool NftControl::blockDns(const std::string &targetIp) {
    
    std::string ruleUdp = "sudo nft add rule inet " + tableName + " " + chainName +
        " ip saddr " + targetIp + " udp dport 53 drop";

    std::string ruleTcp = "sudo nft add rule inet " + tableName + " " + chainName +
        " ip saddr " + targetIp + " tcp dport 53 drop";

    bool ok1 = runCommand(ruleUdp);
    bool ok2 = runCommand(ruleTcp);

    if (ok1 && ok2) {
        log.log("DNS blocked for " + targetIp);
    }

    return ok1 && ok2;
}

bool NftControl::blockConnection(const std::string &targetIp) {
    
    std::string ruleOut = "sudo nft add rule inet " + tableName + " " + chainName +
        " ip saddr " + targetIp + " drop";

    std::string ruleIn = "sudo nft add rule inet " + tableName + " " + chainName +
        " ip daddr " + targetIp + " drop";

    bool ok1 = runCommand(ruleOut);
    bool ok2 = runCommand(ruleIn);

    if (ok1 && ok2) {
        log.log("Connection fully blocked for " + targetIp);
    }

    return ok1 && ok2;
}
