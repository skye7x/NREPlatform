#include "history.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <algorithm>

static const std::string CSV_HEADER =
    "id,name,type,target,start_time,end_time,duration_seconds,status,"
    "avg_latency_ms,packet_loss_pc,jitter_ms,avg_bandwidth_mbps,health_status";

HistoryManager::HistoryManager(Logger &logger, const std::string &path)
    : log(logger), filePath(path) {

    size_t slashPos = filePath.find('/');
    if (slashPos != std::string::npos) {
        std::string folder = filePath.substr(0, slashPos);
        mkdir(folder.c_str(), 0755);
    }

    std::ifstream check(filePath);
    if (!check.good()) {
        std::ofstream newFile(filePath);
        newFile << CSV_HEADER << "\n";
    }
}

std::string HistoryManager::sanitizeForCsv(const std::string &text) {
    std::string result = text;
    std::replace(result.begin(), result.end(), ',', ';');
    return result;
}

int HistoryManager::findNextId() {
    std::vector<ExperimentRecord> existing = loadAll();
    int highest = 0;
    for (const ExperimentRecord &record : existing) {
        if (record.id > highest) highest = record.id;
    }
    return highest + 1;
}

int HistoryManager::addRecord(ExperimentRecord record) {
    record.id = findNextId();

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        log.log("ERROR: could not open history file to save experiment #" + std::to_string(record.id));
        return -1;
    }

    file << record.id << ","
         << sanitizeForCsv(record.name) << ","
         << sanitizeForCsv(record.type) << ","
         << sanitizeForCsv(record.target) << ","
         << sanitizeForCsv(record.startTime) << ","
         << sanitizeForCsv(record.endTime) << ","
         << record.durationSeconds << ","
         << sanitizeForCsv(record.status) << ","
         << record.avgLatencyMs << ","
         << record.packetLossPc << ","
         << record.jitterMs << ","
         << record.avgBandwidthMbps << ","
         << sanitizeForCsv(record.healthStatus)
         << "\n";

    file.close();
    log.log("Experiment #" + std::to_string(record.id) + " saved to history.");
    return record.id;
}

std::vector<ExperimentRecord> HistoryManager::loadAll() {
    std::vector<ExperimentRecord> records;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return records; 
    }

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line)) {
        if (firstLine) {
            firstLine = false;
            continue; 
        }
        if (line.empty()) continue;

        std::vector<std::string> fields;
        std::stringstream stream(line);
        std::string field;
        while (std::getline(stream, field, ',')) {
            fields.push_back(field);
        }
        if (fields.size() < 13) continue; 

        ExperimentRecord record;
        record.id               = std::atoi(fields[0].c_str());
        record.name             = fields[1];
        record.type             = fields[2];
        record.target           = fields[3];
        record.startTime        = fields[4];
        record.endTime          = fields[5];
        record.durationSeconds  = std::atoi(fields[6].c_str());
        record.status           = fields[7];
        record.avgLatencyMs     = std::atof(fields[8].c_str());
        record.packetLossPc     = std::atof(fields[9].c_str());
        record.jitterMs         = std::atof(fields[10].c_str());
        record.avgBandwidthMbps = std::atof(fields[11].c_str());
        record.healthStatus     = fields[12];

        records.push_back(record);
    }

    return records;
}

void HistoryManager::printHistory(int limit) {
    std::vector<ExperimentRecord> records = loadAll();

    if (records.empty()) {
        std::cout << "No experiment history yet.\n";
        return;
    }

    int startIndex = 0;
    if (limit > 0 && (int)records.size() > limit) {
        startIndex = (int)records.size() - limit;
    }

    std::cout << "\nID   Name                     Type            Target            Duration  Status      Health\n";
    std::cout << "---- ------------------------ --------------- ----------------- --------- ----------- --------\n";

    for (int i = startIndex; i < (int)records.size(); ++i) {
        const ExperimentRecord &record = records[i];
        printf("%-4d %-24s %-15s %-17s %-9d %-11s %-8s\n",
               record.id,
               record.name.c_str(),
               record.type.c_str(),
               record.target.c_str(),
               record.durationSeconds,
               record.status.c_str(),
               record.healthStatus.c_str());
    }
    std::cout << std::endl;
}

bool HistoryManager::exportJson(const std::string &outputPath) {
    std::vector<ExperimentRecord> records = loadAll();
    std::ofstream file(outputPath);

    if (!file.is_open()) {
        log.log("ERROR: could not create export file " + outputPath);
        return false;
    }

    file << "[\n";
    for (size_t i = 0; i < records.size(); ++i) {
        const ExperimentRecord &r = records[i];
        file << "  {\n";
        file << "    \"id\": " << r.id << ",\n";
        file << "    \"name\": \"" << r.name << "\",\n";
        file << "    \"type\": \"" << r.type << "\",\n";
        file << "    \"target\": \"" << r.target << "\",\n";
        file << "    \"start_time\": \"" << r.startTime << "\",\n";
        file << "    \"end_time\": \"" << r.endTime << "\",\n";
        file << "    \"duration\": " << r.durationSeconds << ",\n";
        file << "    \"status\": \"" << r.status << "\",\n";
        file << "    \"latency_avg\": " << r.avgLatencyMs << ",\n";
        file << "    \"packet_loss\": " << r.packetLossPc << ",\n";
        file << "    \"jitter\": " << r.jitterMs << ",\n";
        file << "    \"bandwidth_avg\": " << r.avgBandwidthMbps << ",\n";
        file << "    \"health_status\": \"" << r.healthStatus << "\"\n";
        file << "  }" << (i + 1 < records.size() ? "," : "") << "\n";
    }
    file << "]\n";

    file.close();
    log.log("Exported " + std::to_string(records.size()) + " record(s) to " + outputPath + " (JSON)");
    return true;
}

bool HistoryManager::exportCsv(const std::string &outputPath) {
    std::vector<ExperimentRecord> records = loadAll();
    std::ofstream file(outputPath);

    if (!file.is_open()) {
        log.log("ERROR: could not create export file " + outputPath);
        return false;
    }

    file << CSV_HEADER << "\n";
    for (const ExperimentRecord &r : records) {
        file << r.id << "," << r.name << "," << r.type << "," << r.target << ","
             << r.startTime << "," << r.endTime << "," << r.durationSeconds << ","
             << r.status << "," << r.avgLatencyMs << "," << r.packetLossPc << ","
             << r.jitterMs << "," << r.avgBandwidthMbps << "," << r.healthStatus << "\n";
    }

    file.close();
    log.log("Exported " + std::to_string(records.size()) + " record(s) to " + outputPath + " (CSV)");
    return true;
}
