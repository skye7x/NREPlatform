#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include "logger.h"
#include "history.h"

enum class StorageBackend {
    CSV,
    SQLITE
};

class StorageManager {
public:
    StorageManager(Logger &logger, StorageBackend backend = StorageBackend::CSV,
                   const std::string &basePath = "data");

    int addRecord(ExperimentRecord record);
    std::vector<ExperimentRecord> loadAll();
    bool exportJson(const std::string &outputPath);
    bool exportCsv(const std::string &outputPath);

    bool savePolicy(const std::string &name, const std::string &content);
    std::string loadPolicy(const std::string &name);
    std::vector<std::string> listPolicies();
    bool deletePolicy(const std::string &name);

    bool saveProfile(const std::string &name, const std::string &content);
    std::string loadProfile(const std::string &name);
    std::vector<std::string> listProfiles();
    bool deleteProfile(const std::string &name);

    StorageBackend getBackend() const { return backend; }

    bool migrateCsvToSqlite();
    bool migrateSqliteToCsv();

private:
    Logger &log;
    StorageBackend backend;
    std::string basePath;
    std::string dbPath;

    bool sqliteExec(const std::string &sql);
    bool initSqliteDb();

    int csvAddRecord(ExperimentRecord record);
    std::vector<ExperimentRecord> csvLoadAll();
};

#endif
