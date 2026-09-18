#include "tc_control.h"
#include <cstdlib>   
#include <sstream>   

TcControl::TcControl(Logger &logger) : log(logger), active(false) {
}

bool TcControl::runCommand(const std::string &command) {
    log.log("RUN: " + command);

    int result = std::system(command.c_str());

    if (result != 0) {
        log.log("ERROR: command failed (exit code " + std::to_string(result) + ")");
        return false;
    }

    return true;
}

bool TcControl::applyNetem(const std::string &interfaceName, int latencyMs, double packetLossPc) {
    
    std::ostringstream command;
    command << "sudo tc qdisc add dev " << interfaceName << " root netem";

    if (latencyMs > 0) {
        command << " delay " << latencyMs << "ms";
    }

    if (packetLossPc > 0.0) {
        command << " loss " << packetLossPc << "%";
    }

    bool success = runCommand(command.str());

    if (success) {
        active = true;
        log.log("Netem applied on " + interfaceName +
                 " (latency=" + std::to_string(latencyMs) + "ms, loss=" +
                 std::to_string(packetLossPc) + "%)");
    }

    return success;
}

bool TcControl::removeNetem(const std::string &interfaceName) {
    
    std::string command = "sudo tc qdisc del dev " + interfaceName + " root netem";

    bool success = runCommand(command);
    //s
    if (success) {
        active = false;
        log.log("Netem removed from " + interfaceName + " (configuration restored)");
    }

    return success;
}

bool TcControl::isActive() const {
    return active;
}

bool TcControl::applyShapedNetem(const std::string &interfaceName,
                                  const std::string &targetIp,
                                  const std::string &matchField,
                                  int latencyMs,
                                  int jitterMs,
                                  double packetLossPc,
                                  int bandwidthKbit) {

    int effectiveRate = (bandwidthKbit > 0) ? bandwidthKbit : 1000000; 

    std::ostringstream step1;
    step1 << "sudo tc qdisc add dev " << interfaceName << " root handle 1: htb default 30";
    if (!runCommand(step1.str())) return false;

    std::ostringstream step2;
    step2 << "sudo tc class add dev " << interfaceName
          << " parent 1: classid 1:1 htb rate 1000mbit";
    if (!runCommand(step2.str())) return false;

    std::ostringstream step3;
    step3 << "sudo tc class add dev " << interfaceName
          << " parent 1:1 classid 1:10 htb rate " << effectiveRate << "kbit ceil "
          << effectiveRate << "kbit";
    if (!runCommand(step3.str())) return false;

    std::ostringstream step4;
    step4 << "sudo tc class add dev " << interfaceName
          << " parent 1:1 classid 1:30 htb rate 1000mbit";
    if (!runCommand(step4.str())) return false;

    std::ostringstream step5;
    step5 << "sudo tc qdisc add dev " << interfaceName << " parent 1:10 handle 10: netem";
    if (latencyMs > 0) {
        step5 << " delay " << latencyMs << "ms";
        if (jitterMs > 0) {
            step5 << " " << jitterMs << "ms"; 
        }
    }
    if (packetLossPc > 0.0) {
        step5 << " loss " << packetLossPc << "%";
    }
    if (!runCommand(step5.str())) return false;

    if (!targetIp.empty()) {
        std::ostringstream step6;
        step6 << "sudo tc filter add dev " << interfaceName
              << " protocol ip parent 1: prio 1 u32 match ip " << matchField
              << " " << targetIp << "/32 flowid 1:10";
        if (!runCommand(step6.str())) return false;
    }

    active = true;
    log.log("Shaped netem applied on " + interfaceName +
             (targetIp.empty() ? " (whole interface)" : " (target=" + targetIp + ")"));
    return true;
}

bool TcControl::removeShapedNetem(const std::string &interfaceName) {
    
    std::string command = "sudo tc qdisc del dev " + interfaceName + " root";
    bool success = runCommand(command);

    if (success) {
        active = false;
        log.log("Shaped netem removed from " + interfaceName + " (configuration restored)");
    }

    return success;
}

bool TcControl::setupIfbRedirect(const std::string &lanInterface, const std::string &ifbInterface) {
    
    runCommand("sudo modprobe ifb numifbs=1");

    if (!runCommand("sudo ip link set dev " + ifbInterface + " up")) return false;

    if (!runCommand("sudo tc qdisc add dev " + lanInterface + " handle ffff: ingress")) return false;

    std::string filterCmd = "sudo tc filter add dev " + lanInterface +
        " parent ffff: protocol ip u32 match u32 0 0 action mirred egress redirect dev " + ifbInterface;

    bool success = runCommand(filterCmd);
    if (success) {
        log.log("IFB redirect set up: " + lanInterface + " -> " + ifbInterface);
    }
    return success;
}

bool TcControl::teardownIfbRedirect(const std::string &lanInterface, const std::string &ifbInterface) {
    bool ok1 = runCommand("sudo tc qdisc del dev " + lanInterface + " ingress");
    bool ok2 = runCommand("sudo ip link set dev " + ifbInterface + " down");

    if (ok1 && ok2) {
        log.log("IFB redirect removed: " + lanInterface + " -> " + ifbInterface);
    }
    return ok1 && ok2;
}
