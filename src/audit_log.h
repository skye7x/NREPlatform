#ifndef AUDIT_LOG_H
#define AUDIT_LOG_H

#include <string>
#include <vector>
#include "logger.h"

struct AuditEntry {
    int id = 0;
    std::string user;
    std::string action;
    std::string target;
    std::string details;
    std::string timestamp;
};

class AuditLog {
public:
    AuditLog(Logger &logger, const std::string &filePath = "audit/audit.log");

    void record(const std::string &user, const std::string &action,
                const std::string &target, const std::string &details = "");

    std::vector<AuditEntry> loadAll();

    void printLog(int limit = 0);

    bool exportJson(const std::string &outputPath);

private:
    Logger &log;
    std::string filePath;

    std::string currentTimestamp();
    int findNextId();
};

#endif
