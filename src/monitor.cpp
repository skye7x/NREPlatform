#include "monitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <array>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>

NetworkMonitor::NetworkMonitor(Logger &logger,
                                 const std::string &target,
                                 const std::string &interface)
    : log(logger), pingTarget(target), interfaceName(interface) {

    if (pingTarget.empty()) {
        pingTarget = "8.8.8.8";
    }
}

bool NetworkMonitor::pingOnce(double &latencyMs) {
    std::string command = "ping -c 1 -W 1 " + pingTarget + " 2>/dev/null";

    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    std::array<char, 256> lineBuffer;
    std::string output;

    while (fgets(lineBuffer.data(), lineBuffer.size(), pipe) != nullptr) {
        output += lineBuffer.data();
    }
    pclose(pipe);

    size_t timePos = output.find("time=");
    if (timePos == std::string::npos) return false;

    std::string afterTime = output.substr(timePos + 5);
    latencyMs = std::atof(afterTime.c_str());
    return true;
}

long NetworkMonitor::readInterfaceCounter(const std::string &counterName) {
    std::string path = "/sys/class/net/" + interfaceName + "/statistics/" + counterName;
    std::ifstream file(path);

    if (!file.is_open()) return 0;

    long value = 0;
    file >> value;
    return value;
}

double NetworkMonitor::measureDnsResponseTime(const std::string &dnsServer) {
    
    std::string command = "dig @" + dnsServer + " example.com +time=2 +tries=1 2>/dev/null | grep 'Query time:' | awk '{print $4}'";

    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) return -1;

    std::array<char, 64> buffer;
    std::string output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    pclose(pipe);

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }

    if (output.empty()) return -1;
    return std::atof(output.c_str());
}

std::string NetworkMonitor::checkConnectionState() {
    
    std::string path = "/sys/class/net/" + interfaceName + "/operstate";
    std::ifstream file(path);

    if (!file.is_open()) return "UNKNOWN";

    std::string state;
    std::getline(file, state);

    while (!state.empty() && (state.back() == '\n' || state.back() == '\r')) {
        state.pop_back();
    }

    return state.empty() ? "UNKNOWN" : state;
}

void NetworkMonitor::printAsciiChart(const std::vector<MetricSample> &samples, double maxLatency) {
    if (samples.empty()) return;

    const int chartWidth = 50;
    const int chartHeight = 10;

    std::cout << "\nLatency chart:" << std::endl;

    double actualMax = maxLatency;
    if (actualMax <= 0) actualMax = 1.0;

    std::vector<std::vector<char>> grid(chartHeight, std::vector<char>(samples.size(), ' '));

    for (size_t i = 0; i < samples.size(); ++i) {
        if (samples[i].replyReceived) {
            int row = (int)((samples[i].latencyMs / actualMax) * (chartHeight - 1));
            if (row >= chartHeight) row = chartHeight - 1;
            if (row < 0) row = 0;
            grid[chartHeight - 1 - row][i] = '*';
        } else {
            
            grid[chartHeight - 1][i] = 'x';
        }
    }

    for (int row = 0; row < chartHeight; ++row) {
        
        double yValue = actualMax * (chartHeight - 1 - row) / (chartHeight - 1);
        printf("%7.0f |", yValue);

        for (size_t col = 0; col < samples.size(); ++col) {
            std::cout << grid[row][col];
        }
        std::cout << std::endl;
    }

    std::cout << "        +";
    for (size_t i = 0; i < samples.size(); ++i) {
        std::cout << "-";
    }
    std::cout << std::endl;

    std::cout << "         ";
    if (samples.size() <= 30) {
        for (size_t i = 0; i < samples.size(); ++i) {
            std::cout << (i % 10);
        }
    } else {
        std::cout << "0";
        for (size_t i = 1; i < samples.size(); ++i) {
            if (i % 10 == 0) std::cout << (i / 10);
            else std::cout << " ";
        }
    }
    std::cout << " seconds" << std::endl;
}

MetricsResult NetworkMonitor::collectFor(int durationSeconds) {
    std::vector<MetricSample> samples;

    long rxBytesStart = readInterfaceCounter("rx_bytes");
    long txBytesStart = readInterfaceCounter("tx_bytes");

    std::cout << "\nLive monitoring (" << pingTarget << "):" << std::endl;

    for (int second = 1; second <= durationSeconds; ++second) {
        MetricSample sample;
        sample.secondNumber = second;
        sample.replyReceived = pingOnce(sample.latencyMs);
        samples.push_back(sample);

        if (sample.replyReceived) {
            std::cout << "  t+" << second << "s  latency=" << sample.latencyMs << " ms" << std::endl;
        } else {
            std::cout << "  t+" << second << "s  timeout (packet lost)" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    long rxBytesEnd = readInterfaceCounter("rx_bytes");
    long txBytesEnd = readInterfaceCounter("tx_bytes");

    MetricsResult result;
    result.samplesTaken = static_cast<int>(samples.size());
    result.packetsTransmitted = result.samplesTaken;

    std::vector<double> latencies;
    int lostCount = 0;

    for (const MetricSample &sample : samples) {
        if (sample.replyReceived) {
            latencies.push_back(sample.latencyMs);
        } else {
            lostCount++;
        }
    }

    result.packetsDropped = lostCount;

    if (!latencies.empty()) {
        double sum = 0.0;
        result.minLatencyMs = latencies.front();
        result.maxLatencyMs = latencies.front();

        for (double value : latencies) {
            sum += value;
            result.minLatencyMs = std::min(result.minLatencyMs, value);
            result.maxLatencyMs = std::max(result.maxLatencyMs, value);
        }
        result.avgLatencyMs = sum / latencies.size();

        if (latencies.size() > 1) {
            double jitterSum = 0.0;
            for (size_t i = 1; i < latencies.size(); ++i) {
                jitterSum += std::fabs(latencies[i] - latencies[i - 1]);
            }
            result.jitterMs = jitterSum / (latencies.size() - 1);
        }
    }

    if (result.samplesTaken > 0) {
        result.packetLossPc = (double)lostCount / result.samplesTaken * 100.0;
    }

    long totalBytes = (rxBytesEnd - rxBytesStart) + (txBytesEnd - txBytesStart);
    if (durationSeconds > 0 && totalBytes > 0) {
        result.avgBandwidthMbps = (totalBytes * 8.0) / (durationSeconds * 1000000.0);
    }

    result.dnsResponseTimeMs = measureDnsResponseTime();

    result.connectionState = checkConnectionState();

    printAsciiChart(samples, result.maxLatencyMs);

    return result;
}

HealthStatus NetworkMonitor::computeHealthStatus(const MetricsResult &result) {
    if (result.avgLatencyMs >= 300 || result.packetLossPc >= 20 || result.jitterMs >= 100) {
        return HealthStatus::CRITICAL;
    }

    if (result.avgLatencyMs > 100 || result.packetLossPc > 3 || result.jitterMs > 30) {
        return HealthStatus::DEGRADED;
    }

    return HealthStatus::HEALTHY;
}

std::string NetworkMonitor::healthStatusToString(HealthStatus status) {
    switch (status) {
        case HealthStatus::HEALTHY:  return "HEALTHY";
        case HealthStatus::DEGRADED: return "DEGRADED";
        case HealthStatus::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

void NetworkMonitor::printResult(const MetricsResult &result) {
    HealthStatus status = computeHealthStatus(result);

    std::cout << "\n--- Monitoring Summary ---" << std::endl;
    std::cout << "Samples           " << result.samplesTaken << std::endl;
    std::cout << "Packets tx/rx     " << result.packetsTransmitted << " / "
              << (result.packetsTransmitted - result.packetsDropped) << std::endl;
    std::cout << "Packets dropped   " << result.packetsDropped << std::endl;
    std::cout << "Latency (avg)     " << result.avgLatencyMs << " ms" << std::endl;
    std::cout << "Latency (min)     " << result.minLatencyMs << " ms" << std::endl;
    std::cout << "Latency (max)     " << result.maxLatencyMs << " ms" << std::endl;
    std::cout << "Packet loss       " << result.packetLossPc << " %" << std::endl;
    std::cout << "Jitter            " << result.jitterMs << " ms" << std::endl;
    std::cout << "Bandwidth         " << result.avgBandwidthMbps << " Mb/s" << std::endl;
    std::cout << "DNS response      " << (result.dnsResponseTimeMs >= 0 ?
        std::to_string((int)result.dnsResponseTimeMs) + " ms" : "N/A") << std::endl;
    std::cout << "Connection state  " << result.connectionState << std::endl;
    std::cout << "Network Status    " << healthStatusToString(status) << std::endl;
}
