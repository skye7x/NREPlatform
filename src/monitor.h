#ifndef MONITOR_H
#define MONITOR_H

#include <string>
#include <vector>
#include "logger.h"

struct MetricSample {
    int secondNumber;
    bool replyReceived;
    double latencyMs;
};

struct MetricsResult {
    int samplesTaken = 0;
    double avgLatencyMs = 0.0;
    double minLatencyMs = 0.0;
    double maxLatencyMs = 0.0;
    double packetLossPc = 0.0;
    double jitterMs = 0.0;
    double avgBandwidthMbps = 0.0;
    int packetsTransmitted = 0;
    int packetsDropped = 0;
    double dnsResponseTimeMs = 0.0;
    std::string connectionState = "UNKNOWN";
};

enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    CRITICAL
};

class NetworkMonitor {
public:
    NetworkMonitor(Logger &logger,
                    const std::string &pingTarget,
                    const std::string &interfaceName);

    MetricsResult collectFor(int durationSeconds);

    static HealthStatus computeHealthStatus(const MetricsResult &result);
    static std::string healthStatusToString(HealthStatus status);
    static void printResult(const MetricsResult &result);

private:
    Logger &log;
    std::string pingTarget;
    std::string interfaceName;

    bool pingOnce(double &latencyMs);
    long readInterfaceCounter(const std::string &counterName);

    double measureDnsResponseTime(const std::string &dnsServer = "8.8.8.8");

    std::string checkConnectionState();

    void printAsciiChart(const std::vector<MetricSample> &samples, double maxLatency);
};

#endif
