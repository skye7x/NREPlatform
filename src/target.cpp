#include "target.h"
#include <cstdio>
#include <array>
#include <sstream>
#include <algorithm>

TargetSelector::TargetSelector(Logger &logger) : log(logger) {
}

std::string TargetSelector::toLower(const std::string &text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool TargetSelector::isIPv4(const std::string &address) {
    
    bool hasDot = false;
    bool hasDigit = false;
    for (char c : address) {
        if (c == '.') hasDot = true;
        else if (c >= '0' && c <= '9') hasDigit = true;
        else if (c == '/') continue; 
        else return false;
    }
    return hasDot && hasDigit;
}

bool TargetSelector::isIPv6(const std::string &address) {
    
    for (char c : address) {
        if (c == ':') return true;
    }
    return false;
}

bool TargetSelector::isSubnet(const std::string &address) {
    
    return address.find('/') != std::string::npos;
}

std::string TargetSelector::resolveMacToIp(const std::string &macAddress) {
    std::string wantedMac = toLower(macAddress);
    std::string foundIp = "";

    FILE *pipe = popen("ip neigh show", "r");
    if (!pipe) {
        log.log("ERROR: could not run 'ip neigh show' to resolve MAC address.");
        return "";
    }

    std::array<char, 256> lineBuffer;
    while (fgets(lineBuffer.data(), lineBuffer.size(), pipe) != nullptr) {
        std::string line(lineBuffer.data());
        std::string lowerLine = toLower(line);

        if (lowerLine.find(wantedMac) != std::string::npos) {
            std::istringstream stream(line);
            stream >> foundIp;
            break;
        }
    }

    pclose(pipe);

    if (foundIp.empty()) {
        log.log("WARNING: could not resolve MAC " + macAddress + " to an IP address.");
    } else {
        log.log("Resolved MAC " + macAddress + " -> IP " + foundIp);
    }

    return foundIp;
}

std::string TargetSelector::resolveMacToIpv6(const std::string &macAddress) {
    std::string wantedMac = toLower(macAddress);
    std::string foundIpv6 = "";

    FILE *pipe = popen("ip -6 neigh show", "r");
    if (!pipe) {
        log.log("ERROR: could not run 'ip -6 neigh show' to resolve MAC to IPv6.");
        return "";
    }

    std::array<char, 256> lineBuffer;
    while (fgets(lineBuffer.data(), lineBuffer.size(), pipe) != nullptr) {
        std::string line(lineBuffer.data());
        std::string lowerLine = toLower(line);

        if (lowerLine.find(wantedMac) != std::string::npos) {
            std::istringstream stream(line);
            stream >> foundIpv6;
            break;
        }
    }

    pclose(pipe);

    if (foundIpv6.empty()) {
        log.log("WARNING: could not resolve MAC " + macAddress + " to an IPv6 address.");
    } else {
        log.log("Resolved MAC " + macAddress + " -> IPv6 " + foundIpv6);
    }

    return foundIpv6;
}

TargetInfo TargetSelector::parseTarget(const std::string &targetSpec) {
    TargetInfo info;

    if (targetSpec.empty()) return info;

    std::string address = targetSpec;
    std::string portPart;

    size_t udpPos = targetSpec.find(":udp:");
    if (udpPos != std::string::npos) {
        address = targetSpec.substr(0, udpPos);
        portPart = targetSpec.substr(udpPos + 5);
        info.udpPort = std::atoi(portPart.c_str());
        log.log("Parsed UDP port: " + std::to_string(info.udpPort));
    }
    
    else {
        size_t lastColon = targetSpec.rfind(':');
        if (lastColon != std::string::npos) {
            std::string afterColon = targetSpec.substr(lastColon + 1);
            
            bool isPort = !afterColon.empty();
            for (char c : afterColon) {
                if (c < '0' || c > '9') { isPort = false; break; }
            }
            if (isPort) {
                address = targetSpec.substr(0, lastColon);
                info.tcpPort = std::atoi(afterColon.c_str());
                log.log("Parsed TCP port: " + std::to_string(info.tcpPort));
            }
        }
    }

    if (isIPv6(address)) {
        info.ipv6Address = address;
        info.isIPv6 = true;
        log.log("Parsed IPv6 target: " + address);
    } else if (isSubnet(address)) {
        info.subnet = address;
        info.isSubnet = true;
        log.log("Parsed subnet target: " + address);
    } else if (isIPv4(address)) {
        info.ipAddress = address;
        log.log("Parsed IPv4 target: " + address);
    } else {
        
        info.macAddress = address;
        info.ipAddress = resolveMacToIp(address);
        if (!info.ipAddress.empty()) {
            log.log("Resolved MAC target: " + address + " -> " + info.ipAddress);
        }
    }

    return info;
}

std::string TargetSelector::buildTcFilterMatch(const TargetInfo &target, const std::string &direction) {
    std::ostringstream filter;

    if (!target.ipAddress.empty()) {
        filter << "protocol ip parent 1: prio 1 u32 match ip " << direction
               << " " << target.ipAddress << "/32 flowid 1:10";
    } else if (!target.subnet.empty()) {
        filter << "protocol ip parent 1: prio 1 u32 match ip " << direction
               << " " << target.subnet << " flowid 1:10";
    } else if (!target.ipv6Address.empty()) {
        
        filter << "protocol ipv6 parent 1: prio 1 u32 match ip6 " << direction
               << " " << target.ipv6Address << "/128 flowid 1:10";
    }

    return filter.str();
}

std::string TargetSelector::buildNftMatch(const TargetInfo &target) {
    std::ostringstream match;

    if (!target.ipAddress.empty()) {
        if (!target.tcpPort) {
            match << "ip saddr " << target.ipAddress;
        } else {
            match << "ip saddr " << target.ipAddress << " tcp dport " << target.tcpPort;
        }
    } else if (target.udpPort) {
        match << "udp dport " << target.udpPort;
    } else if (!target.ipv6Address.empty()) {
        match << "ip6 saddr " << target.ipv6Address;
    } else if (!target.subnet.empty()) {
        match << "ip saddr " << target.subnet;
    }

    return match.str();
}
