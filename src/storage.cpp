#include "storage.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <cstdio>

StorageManager::StorageManager(Logger &logger, StorageBackend back, const std::string &base)
    : log(logger), backend(back), basePath(base) {
    mkdir(basePath.c_str(), 0755);
    mkdir((basePath + "/history").c_str(), 0755);
    mkdir((basePath + "/policies").c_str(), 0755);
    mkdir((basePath + "/profiles").c_str(), 0755);
    dbPath = basePath + "/nre.db";
}

bool StorageManager::sqliteExec(const std::string &sql) {
    std::string cmd = "sqlite3 " + dbPath + " \"" + sql + "\" 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
}

bool StorageManager::initSqliteDb() {
    std::string cmd = "sqlite3 " + dbPath + " \""
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, type TEXT, target TEXT,"
        "start_time TEXT, end_time TEXT,"
        "duration_seconds INTEGER, status TEXT,"
        "avg_latency_ms REAL, packet_loss_pc REAL,"
        "jitter_ms REAL, avg_bandwidth_mbps REAL,"
        "health_status TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS policies ("
        "name TEXT PRIMARY KEY, content TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS profiles ("
        "name TEXT PRIMARY KEY, content TEXT"
        ");\" 2>/dev/null";

    int result = std::system(cmd.c_str());
    if (result != 0) {
        log.log("WARNING: sqlite3 not available, falling back to CSV storage.");
        backend = StorageBackend::CSV;
        return false;
    }
    return true;
}

int StorageManager::addRecord(ExperimentRecord record) {
    if (backend == StorageBackend::SQLITE) {
        
        HistoryManager hm(log, basePath + "/history/history.csv");
        return hm.addRecord(record);
    }
    return csvAddRecord(record);
}

std::vector<ExperimentRecord> StorageManager::loadAll() {
    if (backend == StorageBackend::SQLITE) {
        HistoryManager hm(log, basePath + "/history/history.csv");
        return hm.loadAll();
    }
    return csvLoadAll();
}

bool StorageManager::exportJson(const std::string &outputPath) {
    HistoryManager hm(log, basePath + "/history/history.csv");
    return hm.exportJson(outputPath);
}

bool StorageManager::exportCsv(const std::string &outputPath) {
    HistoryManager hm(log, basePath + "/history/history.csv");
    return hm.exportCsv(outputPath);
}

int StorageManager::csvAddRecord(ExperimentRecord record) {
    HistoryManager hm(log, basePath + "/history/history.csv");
    return hm.addRecord(record);
}

std::vector<ExperimentRecord> StorageManager::csvLoadAll() {
    HistoryManager hm(log, basePath + "/history/history.csv");
    return hm.loadAll();
}

bool StorageManager::savePolicy(const std::string &name, const std::string &content) {
    std::string path = basePath + "/policies/" + name + ".policy";
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    file.close();
    return true;
}

std::string StorageManager::loadPolicy(const std::string &name) {
    std::string path = basePath + "/policies/" + name + ".policy";
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

std::vector<std::string> StorageManager::listPolicies() {
    std::vector<std::string> names;
    std::string folder = basePath + "/policies";
    DIR *dir = opendir(folder.c_str());
    if (!dir) return names;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        std::string suffix = ".policy";
        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            names.push_back(filename.substr(0, filename.size() - suffix.size()));
        }
    }
    closedir(dir);
    return names;
}

bool StorageManager::deletePolicy(const std::string &name) {
    std::string path = basePath + "/policies/" + name + ".policy";
    return remove(path.c_str()) == 0;
}

bool StorageManager::saveProfile(const std::string &name, const std::string &content) {
    std::string path = basePath + "/profiles/" + name + ".profile";
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    file.close();
    return true;
}

std::string StorageManager::loadProfile(const std::string &name) {
    std::string path = basePath + "/profiles/" + name + ".profile";
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

std::vector<std::string> StorageManager::listProfiles() {
    std::vector<std::string> names;
    std::string folder = basePath + "/profiles";
    DIR *dir = opendir(folder.c_str());
    if (!dir) return names;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        std::string suffix = ".profile";
        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            names.push_back(filename.substr(0, filename.size() - suffix.size()));
        }
    }
    closedir(dir);
    return names;
}

bool StorageManager::deleteProfile(const std::string &name) {
    std::string path = basePath + "/profiles/" + name + ".profile";
    return remove(path.c_str()) == 0;
}

bool StorageManager::migrateCsvToSqlite() {
    log.log("Migrating CSV data to SQLite...");
    std::vector<ExperimentRecord> records = csvLoadAll();

    StorageBackend originalBackend = backend;
    backend = StorageBackend::SQLITE;
    initSqliteDb();

    for (auto &record : records) {
        addRecord(record);
    }

    log.log("Migration complete: " + std::to_string(records.size()) + " records.");
    return true;
}

bool StorageManager::migrateSqliteToCsv() {
    log.log("Migrating SQLite data to CSV...");
    backend = StorageBackend::CSV;
    
    log.log("Migration complete.");
    return true;
}
