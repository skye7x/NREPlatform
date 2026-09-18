#ifndef CONDITION_H
#define CONDITION_H

#include <string>
#include "monitor.h" 

struct Condition {
    std::string metric;      
    std::string op;          
    double value = 0.0;      

    bool isEmpty() const {
        return metric.empty();
    }

    static double extractMetricValue(const MetricsResult &metrics, const std::string &metricName) {
        if (metricName == "latency")     return metrics.avgLatencyMs;
        if (metricName == "packet_loss") return metrics.packetLossPc;
        if (metricName == "jitter")      return metrics.jitterMs;
        if (metricName == "bandwidth")   return metrics.avgBandwidthMbps;
        return 0.0; 
    }

    bool evaluate(const MetricsResult &metrics) const {
        double actual = extractMetricValue(metrics, metric);

        if (op == ">")  return actual >  value;
        if (op == "<")  return actual <  value;
        if (op == ">=") return actual >= value;
        if (op == "<=") return actual <= value;
        if (op == "==") return actual == value;

        return false; 
    }
};

#endif
