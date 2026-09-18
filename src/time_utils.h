#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <ctime>

inline std::string nowTimestamp() {
    time_t now = time(nullptr);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buffer);
}

#endif
