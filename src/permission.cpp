#include "permission.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <functional>

PermissionManager::PermissionManager(Logger &logger, const std::string &folderName)
    : log(logger), folder(folderName) {
    mkdir(folder.c_str(), 0755);

    DIR *dir = opendir(folder.c_str());
    if (dir) {
        int count = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name.find(".user") != std::string::npos) count++;
        }
        closedir(dir);

        if (count == 0) {
            addUser("admin", "admin", PermissionLevel::ADMIN);
            log.log("Default admin user created (username: admin, password: admin)");
        }
    }
}

std::string PermissionManager::hashPassword(const std::string &password) {
    
    std::hash<std::string> hasher;
    size_t hash = hasher(password + "nre_salt_2024");
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

std::string PermissionManager::permissionLevelToString(PermissionLevel level) {
    switch (level) {
        case PermissionLevel::VIEWER:  return "viewer";
        case PermissionLevel::OPERATOR: return "operator";
        case PermissionLevel::ADMIN:   return "admin";
    }
    return "unknown";
}

bool PermissionManager::addUser(const std::string &username, const std::string &password, PermissionLevel level) {
    User user;
    user.username = username;
    user.passwordHash = hashPassword(password);
    user.level = level;
    user.enabled = true;

    if (saveUser(user)) {
        log.log("User added: " + username + " (level: " + permissionLevelToString(level) + ")");
        return true;
    }
    return false;
}

bool PermissionManager::removeUser(const std::string &username) {
    std::string path = folder + "/" + username + ".user";
    if (remove(path.c_str()) == 0) {
        log.log("User removed: " + username);
        return true;
    }
    log.log("ERROR: could not remove user " + username);
    return false;
}

bool PermissionManager::authenticate(const std::string &username, const std::string &password) {
    bool found = false;
    User user = loadUser(username, found);

    if (!found) {
        log.log("Authentication failed: user " + username + " not found.");
        return false;
    }

    if (!user.enabled) {
        log.log("Authentication failed: user " + username + " is disabled.");
        return false;
    }

    if (user.passwordHash == hashPassword(password)) {
        log.log("Authentication successful: " + username);
        return true;
    }

    log.log("Authentication failed: wrong password for " + username);
    return false;
}

bool PermissionManager::hasPermission(const std::string &username, PermissionLevel requiredLevel) {
    PermissionLevel userLevel = getUserLevel(username);
    return static_cast<int>(userLevel) >= static_cast<int>(requiredLevel);
}

PermissionLevel PermissionManager::getUserLevel(const std::string &username) {
    bool found = false;
    User user = loadUser(username, found);
    if (!found) return PermissionLevel::VIEWER;
    return user.level;
}

std::vector<User> PermissionManager::listUsers() {
    std::vector<User> users;

    DIR *dir = opendir(folder.c_str());
    if (!dir) return users;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        std::string suffix = ".user";

        if (filename.size() > suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            std::string username = filename.substr(0, filename.size() - suffix.size());
            bool found = false;
            User user = loadUser(username, found);
            if (found) users.push_back(user);
        }
    }

    closedir(dir);
    return users;
}

bool PermissionManager::changePassword(const std::string &username, const std::string &oldPassword, const std::string &newPassword) {
    if (!authenticate(username, oldPassword)) {
        return false;
    }

    bool found = false;
    User user = loadUser(username, found);
    if (!found) return false;

    user.passwordHash = hashPassword(newPassword);
    if (saveUser(user)) {
        log.log("Password changed for user: " + username);
        return true;
    }
    return false;
}

bool PermissionManager::saveUser(const User &user) {
    std::string path = folder + "/" + user.username + ".user";
    std::ofstream file(path);

    if (!file.is_open()) {
        log.log("ERROR: could not save user to " + path);
        return false;
    }

    file << "username=" << user.username << "\n";
    file << "password_hash=" << user.passwordHash << "\n";
    file << "level=" << permissionLevelToString(user.level) << "\n";
    file << "enabled=" << (user.enabled ? "1" : "0") << "\n";

    file.close();
    return true;
}

User PermissionManager::loadUser(const std::string &username, bool &found) {
    User user;
    user.username = username;

    std::string path = folder + "/" + username + ".user";
    std::ifstream file(path);

    if (!file.is_open()) {
        found = false;
        return user;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "password_hash") user.passwordHash = value;
        else if (key == "level") {
            if (value == "admin") user.level = PermissionLevel::ADMIN;
            else if (value == "operator") user.level = PermissionLevel::OPERATOR;
            else user.level = PermissionLevel::VIEWER;
        }
        else if (key == "enabled") user.enabled = (value == "1");
    }

    found = true;
    return user;
}
