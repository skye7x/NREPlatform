#ifndef HISTORY_H
#define HISTORY_H

#include <string>
#include <vector>
#include "logger.h"

struct ExperimentRecord {
    int id = 0;
    std::string name;
    std::string type;          
    std::string target;        
    std::string startTime;
    std::string endTime;
    int durationSeconds = 0;
    std::string status;        

    double avgLatencyMs = 0.0;
    double packetLossPc = 0.0;
    double jitterMs = 0.0;
    double avgBandwidthMbps = 0.0;
    std::string healthStatus = "N/A";
};

class HistoryManager {
public:
    HistoryManager(Logger &logger, const std::string &filePath = "history/history.csv");

    int addRecord(ExperimentRecord record);

    std::vector<ExperimentRecord> loadAll();

    void printHistory(int limit = 0); 

    bool exportJson(const std::string &outputPath);
    bool exportCsv(const std::string &outputPath);

private:
    Logger &log;
    std::string filePath;

    int findNextId();

    std::string sanitizeForCsv(const std::string &text);
};

#endif
