#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

class Logger {
public:
    
    Logger(const std::string &filename = "nre.log");

    ~Logger();

    void log(const std::string &message);

private:
    std::ofstream logFile;

    std::string currentTimestamp();
};

#endif
