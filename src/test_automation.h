#ifndef TEST_AUTOMATION_H
#define TEST_AUTOMATION_H

#include <string>
#include <vector>
#include <functional>
#include "logger.h"
#include "profile.h"
#include "monitor.h"
#include "tc_control.h"
#include "nft_control.h"
#include "target.h"
#include "resource_guard.h"

struct TestStep {
    std::string action;
    std::string target;
    int durationSeconds = 0;
    std::string parameters;
};

struct TestScenario {
    std::string name;
    std::string description;
    std::vector<TestStep> steps;
};

struct TestResult {
    std::string scenarioName;
    std::string startTime;
    std::string endTime;
    bool success = false;
    MetricsResult metrics;
    std::string exportPath;
};

class TestAutomation {
public:
    TestAutomation(Logger &logger, TcControl &tc, NftControl &nft,
                   TargetSelector &ts, ResourceGuard *guard = nullptr);

    bool runScenario(const TestScenario &scenario, const std::string &interfaceName = "eth0");

    TestScenario createStandardTest(const std::string &profileName,
                                    const std::string &interfaceName = "eth0",
                                    int experimentDuration = 60,
                                    int waitBeforeMetrics = 5);

    bool exportResults(const TestResult &result, const std::string &format);

    std::vector<std::string> listScenarios();

    using HistoryCallback = std::function<int(ExperimentRecord &)>;
    void setHistoryCallback(HistoryCallback cb);

private:
    Logger &log;
    TcControl &tcControl;
    NftControl &nftControl;
    TargetSelector &targetSelector;
    ResourceGuard *guard;
    HistoryCallback historyCb;

    bool startApp(const std::string &appPath);
    bool stopApp(const std::string &appPath);
    bool runExperimentStep(const TestStep &step, MetricsResult &result, const std::string &interfaceName);
    bool waitSeconds(int seconds);
    bool exportMetrics(const MetricsResult &result, const std::string &path, const std::string &format);
};

#endif
