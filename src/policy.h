#ifndef POLICY_H
#define POLICY_H

#include <string>
#include <vector>
#include "logger.h"
#include "condition.h"

const int MAX_CONDITIONS = 5;

struct Policy {
    std::string name;
    std::vector<Condition> conditions;
    std::string logic = "AND";   
    std::string action;          
    std::string actionParam;     
};

class PolicyManager {
public:
    PolicyManager(Logger &logger, const std::string &folder = "policies");

    bool savePolicy(const Policy &policy);
    Policy loadPolicy(const std::string &name, bool &found);
    std::vector<std::string> listPolicyNames();

    std::vector<Policy> loadAllPolicies();

private:
    Logger &log;
    std::string folder;
};

#endif
