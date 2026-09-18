#include "audit_log.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <ctime>

AuditLog::AuditLog(Logger &logger, const std::string &path)
    : log(logger), filePath(path) {
    size_t slashPos = filePath.find('/');
    if (slashPos != std::string::npos) {
        std::string folder = filePath.substr(0, slashPos);
        mkdir(folder.c_str(), 0755);
    }
}

std::string AuditLog::currentTimestamp() {
    time_t now = time(nullptr);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buffer);
}

int AuditLog::findNextId() {
    std::vector<AuditEntry> existing = loadAll();
    int highest = 0;
    for (const AuditEntry &entry : existing) {
        if (entry.id > highest) highest = entry.id;
    }
    return highest + 1;
}

void AuditLog::record(const std::string &user, const std::string &action,
                       const std::string &target, const std::string &details) {
    AuditEntry entry;
    entry.id = findNextId();
    entry.user = user;
    entry.action = action;
    entry.target = target;
    entry.details = details;
    entry.timestamp = currentTimestamp();

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        log.log("ERROR: could not open audit log file.");
        return;
    }

    file << entry.id << "|"
         << entry.user << "|"
         << entry.action << "|"
         << entry.target << "|"
         << entry.details << "|"
         << entry.timestamp << "\n";

    file.close();
    log.log("AUDIT: [" + entry.timestamp + "] " + entry.user + " " + entry.action +
            " " + entry.target + (details.empty() ? "" : " (" + details + ")"));
}

std::vector<AuditEntry> AuditLog::loadAll() {
    std::vector<AuditEntry> entries;
    std::ifstream file(filePath);

    if (!file.is_open()) return entries;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<std::string> fields;
        std::istringstream iss(line);
        std::string field;
        while (std::getline(iss, field, '|')) {
            fields.push_back(field);
        }

        if (fields.size() < 5) continue;

        AuditEntry entry;
        entry.id = std::atoi(fields[0].c_str());
        entry.user = fields[1];
        entry.action = fields[2];
        entry.target = fields[3];
        entry.details = fields[4];
        entry.timestamp = fields.size() > 5 ? fields[5] : "";

        entries.push_back(entry);
    }

    return entries;
}

void AuditLog::printLog(int limit) {
    std::vector<AuditEntry> entries = loadAll();

    if (entries.empty()) {
        std::cout << "No audit entries yet.\n";
        return;
    }

    int startIndex = 0;
    if (limit > 0 && (int)entries.size() > limit) {
        startIndex = (int)entries.size() - limit;
    }

    std::cout << "\nID   Timestamp            User         Action           Target           Details\n";
    std::cout << "---- -------------------- ------------ ---------------- ---------------- ----------------\n";

    for (int i = startIndex; i < (int)entries.size(); ++i) {
        const AuditEntry &e = entries[i];
        printf("%-4d %-20s %-12s %-16s %-16s %-16s\n",
               e.id, e.timestamp.c_str(), e.user.c_str(), e.action.c_str(),
               e.target.c_str(), e.details.c_str());
    }
    std::cout << std::endl;
}

bool AuditLog::exportJson(const std::string &outputPath) {
    std::vector<AuditEntry> entries = loadAll();
    std::ofstream file(outputPath);

    if (!file.is_open()) {
        log.log("ERROR: could not create audit export file " + outputPath);
        return false;
    }

    file << "[\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        const AuditEntry &e = entries[i];
        file << "  {\n";
        file << "    \"id\": " << e.id << ",\n";
        file << "    \"user\": \"" << e.user << "\",\n";
        file << "    \"action\": \"" << e.action << "\",\n";
        file << "    \"target\": \"" << e.target << "\",\n";
        file << "    \"details\": \"" << e.details << "\",\n";
        file << "    \"timestamp\": \"" << e.timestamp << "\"\n";
        file << "  }" << (i + 1 < entries.size() ? "," : "") << "\n";
    }
    file << "]\n";

    file.close();
    log.log("Audit log exported to " + outputPath);
    return true;
}
