#include "test_automation.h"
#include "experiment.h"
#include "time_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <algorithm>

TestAutomation::TestAutomation(Logger &logger, TcControl &tc, NftControl &nft,
                                 TargetSelector &ts, ResourceGuard *rg)
    : log(logger), tcControl(tc), nftControl(nft), targetSelector(ts), guard(rg) {
}

void TestAutomation::setHistoryCallback(HistoryCallback cb) {
    historyCb = cb;
}

bool TestAutomation::runScenario(const TestScenario &scenario, const std::string &interfaceName) {
    std::cout << "\n=== Test Automation: " << scenario.name << " ===" << std::endl;
    std::cout << "Description: " << scenario.description << std::endl;
    std::cout << "Steps: " << scenario.steps.size() << "\n" << std::endl;

    log.log("Test scenario started: " + scenario.name);

    TestResult result;
    result.scenarioName = scenario.name;
    result.startTime = nowTimestamp();
    result.success = true;

    for (size_t i = 0; i < scenario.steps.size(); ++i) {
        const TestStep &step = scenario.steps[i];
        std::cout << "Step " << (i + 1) << "/" << scenario.steps.size()
                  << ": " << step.action << " -> " << step.target << std::endl;

        log.log("Test step " + std::to_string(i + 1) + ": " + step.action + " " + step.target);

        bool stepSuccess = false;

        if (step.action == "start_app") {
            stepSuccess = startApp(step.target);
        } else if (step.action == "stop_app") {
            stepSuccess = stopApp(step.target);
        } else if (step.action == "run_experiment") {
            stepSuccess = runExperimentStep(step, result.metrics, interfaceName);
        } else if (step.action == "wait") {
            stepSuccess = waitSeconds(step.durationSeconds);
        } else if (step.action == "collect") {
            stepSuccess = true;
        } else if (step.action == "export") {
            stepSuccess = exportMetrics(result.metrics, step.target, step.parameters);
            result.exportPath = step.target;
        } else {
            std::cout << "  Unknown action: " << step.action << std::endl;
            stepSuccess = false;
        }

        if (!stepSuccess) {
            std::cout << "  Step FAILED!" << std::endl;
            log.log("Test step " + std::to_string(i + 1) + " failed.");
            result.success = false;
            break;
        }

        std::cout << "  Step OK." << std::endl;
    }

    result.endTime = nowTimestamp();

    if (historyCb && result.success) {
        ExperimentRecord record;
        record.name = "test_" + scenario.name;
        record.type = "test_automation";
        record.target = "automated";
        record.startTime = result.startTime;
        record.endTime = result.endTime;
        record.durationSeconds = 0;
        record.status = result.success ? "completed" : "aborted";
        record.avgLatencyMs = result.metrics.avgLatencyMs;
        record.packetLossPc = result.metrics.packetLossPc;
        record.jitterMs = result.metrics.jitterMs;
        record.avgBandwidthMbps = result.metrics.avgBandwidthMbps;
        record.healthStatus = NetworkMonitor::healthStatusToString(
            NetworkMonitor::computeHealthStatus(result.metrics));
        historyCb(record);
    }

    std::cout << "\n=== Test " << (result.success ? "PASSED" : "FAILED") << " ===\n" << std::endl;
    log.log("Test scenario finished: " + scenario.name + " (" +
            (result.success ? "PASSED" : "FAILED") + ")");

    return result.success;
}

TestScenario TestAutomation::createStandardTest(const std::string &profileName,
                                                  const std::string &interfaceName,
                                                  int experimentDuration,
                                                  int waitBeforeMetrics) {
    TestScenario scenario;
    scenario.name = "standard_test_" + profileName;
    scenario.description = "Standard test for profile: " + profileName;

    TestStep waitStep;
    waitStep.action = "wait";
    waitStep.durationSeconds = waitBeforeMetrics;
    scenario.steps.push_back(waitStep);

    TestStep experimentStep;
    experimentStep.action = "run_experiment";
    experimentStep.target = profileName;
    experimentStep.parameters = interfaceName;
    experimentStep.durationSeconds = experimentDuration;
    scenario.steps.push_back(experimentStep);

    TestStep exportStep;
    exportStep.action = "export";
    exportStep.target = "results/" + profileName + "_result.json";
    exportStep.parameters = "json";
    scenario.steps.push_back(exportStep);

    return scenario;
}

bool TestAutomation::startApp(const std::string &appPath) {
    std::cout << "  Starting application: " << appPath << std::endl;
    log.log("Starting app: " + appPath);
    int result = std::system(("nohup " + appPath + " > /dev/null 2>&1 &").c_str());
    return result == 0;
}

bool TestAutomation::stopApp(const std::string &appPath) {
    std::cout << "  Stopping application: " << appPath << std::endl;
    log.log("Stopping app: " + appPath);
    size_t lastSlash = appPath.rfind('/');
    std::string appName = (lastSlash != std::string::npos) ? appPath.substr(lastSlash + 1) : appPath;
    std::system(("pkill -f " + appName).c_str());
    return true;
}

bool TestAutomation::runExperimentStep(const TestStep &step, MetricsResult &result, const std::string &interfaceName) {
    std::cout << "  Running REAL experiment: " << step.target
              << " for " << step.durationSeconds << " seconds" << std::endl;
    log.log("Running real experiment: " + step.target);

    Experiment experiment(step.target, interfaceName,
                           "", "",  
                           100,     
                           20,      
                           2.0,     
                           0,       
                           0,       
                           step.durationSeconds,
                           log, tcControl, targetSelector, guard);

    experiment.printSummary();
    result = experiment.run();

    std::cout << "  Experiment completed. Metrics:" << std::endl;
    std::cout << "    Latency: " << result.avgLatencyMs << " ms" << std::endl;
    std::cout << "    Loss: " << result.packetLossPc << " %" << std::endl;
    std::cout << "    Jitter: " << result.jitterMs << " ms" << std::endl;
    std::cout << "    Bandwidth: " << result.avgBandwidthMbps << " Mb/s" << std::endl;

    return experiment.wasSuccessful();
}

bool TestAutomation::waitSeconds(int seconds) {
    std::cout << "  Waiting " << seconds << " seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    return true;
}

bool TestAutomation::exportMetrics(const MetricsResult &result, const std::string &path, const std::string &format) {
    std::cout << "  Exporting metrics to: " << path << " (" << format << ")" << std::endl;
    log.log("Exporting metrics to " + path);

    std::string dir = path.substr(0, path.rfind('/'));
    if (!dir.empty()) std::system(("mkdir -p " + dir).c_str());

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cout << "  Failed to create export file." << std::endl;
        return false;
    }

    if (format == "json") {
        file << "{\n";
        file << "  \"samples\": " << result.samplesTaken << ",\n";
        file << "  \"latency_avg\": " << result.avgLatencyMs << ",\n";
        file << "  \"latency_min\": " << result.minLatencyMs << ",\n";
        file << "  \"latency_max\": " << result.maxLatencyMs << ",\n";
        file << "  \"packet_loss\": " << result.packetLossPc << ",\n";
        file << "  \"jitter\": " << result.jitterMs << ",\n";
        file << "  \"bandwidth\": " << result.avgBandwidthMbps << "\n";
        file << "}\n";
    } else {
        file << "samples,latency_avg,latency_min,latency_max,packet_loss,jitter,bandwidth\n";
        file << result.samplesTaken << ","
             << result.avgLatencyMs << ","
             << result.minLatencyMs << ","
             << result.maxLatencyMs << ","
             << result.packetLossPc << ","
             << result.jitterMs << ","
             << result.avgBandwidthMbps << "\n";
    }

    file.close();
    return true;
}

std::vector<std::string> TestAutomation::listScenarios() {
    std::vector<std::string> scenarios;
    scenarios.push_back("standard_test");
    scenarios.push_back("stress_test");
    scenarios.push_back("endurance_test");
    return scenarios;
}

bool TestAutomation::exportResults(const TestResult &result, const std::string &format) {
    std::string path = "results/" + result.scenarioName + "_result." + format;
    std::cout << "Exporting test results to: " << path << " (" << format << ")" << std::endl;
    log.log("Exporting test results to " + path);

    std::system("mkdir -p results");

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed to create export file: " << path << std::endl;
        return false;
    }

    if (format == "json") {
        file << "{\n";
        file << "  \"scenario\": \"" << result.scenarioName << "\",\n";
        file << "  \"start\": \"" << result.startTime << "\",\n";
        file << "  \"end\": \"" << result.endTime << "\",\n";
        file << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
        file << "  \"metrics\": {\n";
        file << "    \"samples\": " << result.metrics.samplesTaken << ",\n";
        file << "    \"latency_avg\": " << result.metrics.avgLatencyMs << ",\n";
        file << "    \"latency_min\": " << result.metrics.minLatencyMs << ",\n";
        file << "    \"latency_max\": " << result.metrics.maxLatencyMs << ",\n";
        file << "    \"packet_loss\": " << result.metrics.packetLossPc << ",\n";
        file << "    \"jitter\": " << result.metrics.jitterMs << ",\n";
        file << "    \"bandwidth\": " << result.metrics.avgBandwidthMbps << "\n";
        file << "  },\n";
        file << "  \"export_path\": \"" << result.exportPath << "\"\n";
        file << "}\n";
    } else {
        file << "scenario,start,end,success,samples,latency_avg,latency_min,latency_max,packet_loss,jitter,bandwidth\n";
        file << result.scenarioName << ","
             << result.startTime << ","
             << result.endTime << ","
             << (result.success ? "true" : "false") << ","
             << result.metrics.samplesTaken << ","
             << result.metrics.avgLatencyMs << ","
             << result.metrics.minLatencyMs << ","
             << result.metrics.maxLatencyMs << ","
             << result.metrics.packetLossPc << ","
             << result.metrics.jitterMs << ","
             << result.metrics.avgBandwidthMbps << "\n";
    }

    file.close();
    std::cout << "Results exported to: " << path << std::endl;
    return true;
}
