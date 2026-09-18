#ifndef API_H
#define API_H

#include <string>
#include <thread>
#include <atomic>
#include "logger.h"
#include "tc_control.h"
#include "nft_control.h"
#include "target.h"
#include "policy.h"
#include "profile.h"
#include "history.h"
#include "resource_guard.h"

class RestApi {
public:
    RestApi(Logger &logger, int port = 8080);
    ~RestApi();

    bool start();

    bool stop();

    bool isRunning() const;

private:
    Logger &log;
    int port;
    std::thread serverThread;
    std::atomic<bool> running;
    int serverSocket;

    void serverLoop();
    std::string handleRequest(const std::string &method, const std::string &path, const std::string &body);
    std::string handleGetExperiments();
    std::string handlePostExperiment(const std::string &body);
    std::string handleGetProfiles();
    std::string handlePostProfile(const std::string &body);
    std::string handleGetPolicies();
    std::string handlePostPolicy(const std::string &body);
    std::string handleGetHistory();
    std::string handleGetStatus();
    std::string buildJsonResponse(int statusCode, const std::string &json);
    std::string buildHtmlDashboard();
};

#endif
