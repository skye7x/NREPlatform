#ifndef PERMISSION_H
#define PERMISSION_H

#include <string>
#include <vector>
#include "logger.h"

enum class PermissionLevel {
    VIEWER,      
    OPERATOR,    
    ADMIN        
};

struct User {
    std::string username;
    std::string passwordHash;  
    PermissionLevel level;
    bool enabled = true;
};

class PermissionManager {
public:
    PermissionManager(Logger &logger, const std::string &folder = "users");

    bool addUser(const std::string &username, const std::string &password, PermissionLevel level);

    bool removeUser(const std::string &username);

    bool authenticate(const std::string &username, const std::string &password);

    bool hasPermission(const std::string &username, PermissionLevel requiredLevel);

    PermissionLevel getUserLevel(const std::string &username);

    std::vector<User> listUsers();

    bool changePassword(const std::string &username, const std::string &oldPassword, const std::string &newPassword);

    static std::string hashPassword(const std::string &password);

private:
    Logger &log;
    std::string folder;

    bool saveUser(const User &user);
    User loadUser(const std::string &username, bool &found);

    static std::string permissionLevelToString(PermissionLevel level);
};

#endif
