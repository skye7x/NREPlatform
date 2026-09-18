#include "logger.h"
#include <iostream>
#include <ctime>

Logger::Logger(const std::string &filename) {
    
    logFile.open(filename, std::ios::app);

    if (!logFile.is_open()) {
        std::cout << "WARNING: could not open log file " << filename << std::endl;
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::currentTimestamp() {
    time_t now = time(nullptr);
    char buffer[32];

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

    return std::string(buffer);
}

void Logger::log(const std::string &message) {
    std::string line = currentTimestamp() + "  " + message;

    std::cout << line << std::endl;

    if (logFile.is_open()) {
        logFile << line << std::endl;
        logFile.flush(); 
    }
}
