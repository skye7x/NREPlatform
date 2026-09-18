#include "api.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

RestApi::RestApi(Logger &logger, int portNum)
    : log(logger), port(portNum), running(false), serverSocket(-1) {
}

RestApi::~RestApi() {
    stop();
}

bool RestApi::start() {
    if (running) return false;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        log.log("ERROR: could not create API server socket.");
        return false;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log.log("ERROR: could not bind API server to port " + std::to_string(port));
        close(serverSocket);
        return false;
    }

    if (listen(serverSocket, 5) < 0) {
        log.log("ERROR: could not listen on API server socket.");
        close(serverSocket);
        return false;
    }

    running = true;
    serverThread = std::thread(&RestApi::serverLoop, this);

    log.log("REST API server started on port " + std::to_string(port));
    std::cout << "REST API server started on port " << port << std::endl;

    return true;
}

bool RestApi::stop() {
    if (!running) return true;

    running = false;

    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }

    if (serverThread.joinable()) {
        serverThread.join();
    }

    log.log("REST API server stopped.");
    return true;
}

bool RestApi::isRunning() const {
    return running;
}

void RestApi::serverLoop() {
    while (running) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) continue;

        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            std::string request(buffer);
            std::string method, path, body;

            std::istringstream iss(request);
            iss >> method >> path;

            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                body = request.substr(bodyPos + 4);
            }

            std::string response = handleRequest(method, path, body);

            write(clientSocket, response.c_str(), response.size());
        }

        close(clientSocket);
    }
}

std::string RestApi::handleRequest(const std::string &method, const std::string &path, const std::string &body) {
    log.log("API: " + method + " " + path);

    if (method == "GET" && path == "/api/status") {
        return handleGetStatus();
    } else if (method == "GET" && path == "/api/experiments") {
        return handleGetExperiments();
    } else if (method == "POST" && path == "/api/experiments") {
        return handlePostExperiment(body);
    } else if (method == "GET" && path == "/api/profiles") {
        return handleGetProfiles();
    } else if (method == "POST" && path == "/api/profiles") {
        return handlePostProfile(body);
    } else if (method == "GET" && path == "/api/policies") {
        return handleGetPolicies();
    } else if (method == "POST" && path == "/api/policies") {
        return handlePostPolicy(body);
    } else if (method == "GET" && path == "/api/history") {
        return handleGetHistory();
    } else if (method == "GET" && path == "/") {
        return buildJsonResponse(200, buildHtmlDashboard());
    } else {
        return buildJsonResponse(404, "{\"error\": \"Not found\"}");
    }
}

std::string RestApi::buildJsonResponse(int statusCode, const std::string &json) {
    std::ostringstream response;
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 201: statusText = "Created"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Unknown"; break;
    }

    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Content-Length: " << json.size() << "\r\n";
    response << "\r\n";
    response << json;

    return response.str();
}

std::string RestApi::handleGetStatus() {
    std::ostringstream json;
    json << "{\n";
    json << "  \"status\": \"running\",\n";
    json << "  \"version\": \"1.0.0\",\n";
    json << "  \"uptime\": " << time(nullptr) << "\n";
    json << "}\n";
    return buildJsonResponse(200, json.str());
}

std::string RestApi::handleGetExperiments() {
    
    Logger tmpLog("/dev/null");
    ProfileManager pm(tmpLog);
    std::vector<std::string> profiles = pm.listProfiles();

    std::ostringstream json;
    json << "[\n";
    for (size_t i = 0; i < profiles.size(); ++i) {
        json << "  {\"name\": \"" << profiles[i] << "\"}";
        if (i + 1 < profiles.size()) json << ",";
        json << "\n";
    }
    json << "]\n";

    return buildJsonResponse(200, json.str());
}

std::string RestApi::handlePostExperiment(const std::string &body) {
    
    log.log("API: experiment request received");
    return buildJsonResponse(201, "{\"message\": \"Experiment queued\", \"body\": \"" + body + "\"}");
}

std::string RestApi::handleGetProfiles() {
    Logger tmpLog("/dev/null");
    ProfileManager pm(tmpLog);
    std::vector<std::string> profiles = pm.listProfiles();

    std::ostringstream json;
    json << "[\n";
    for (size_t i = 0; i < profiles.size(); ++i) {
        json << "  {\"name\": \"" << profiles[i] << "\"}";
        if (i + 1 < profiles.size()) json << ",";
        json << "\n";
    }
    json << "]\n";

    return buildJsonResponse(200, json.str());
}

std::string RestApi::handlePostProfile(const std::string &body) {
    log.log("API: profile creation request received");
    return buildJsonResponse(201, "{\"message\": \"Profile created\"}");
}

std::string RestApi::handleGetPolicies() {
    Logger tmpLog("/dev/null");
    PolicyManager pm(tmpLog);
    std::vector<std::string> policies = pm.listPolicyNames();

    std::ostringstream json;
    json << "[\n";
    for (size_t i = 0; i < policies.size(); ++i) {
        json << "  {\"name\": \"" << policies[i] << "\"}";
        if (i + 1 < policies.size()) json << ",";
        json << "\n";
    }
    json << "]\n";

    return buildJsonResponse(200, json.str());
}

std::string RestApi::handlePostPolicy(const std::string &body) {
    log.log("API: policy creation request received");
    return buildJsonResponse(201, "{\"message\": \"Policy created\"}");
}

std::string RestApi::handleGetHistory() {
    Logger tmpLog("/dev/null");
    HistoryManager hm(tmpLog);
    std::vector<ExperimentRecord> records = hm.loadAll();

    std::ostringstream json;
    json << "[\n";
    for (size_t i = 0; i < records.size(); ++i) {
        const ExperimentRecord &r = records[i];
        json << "  {\n";
        json << "    \"id\": " << r.id << ",\n";
        json << "    \"name\": \"" << r.name << "\",\n";
        json << "    \"type\": \"" << r.type << "\",\n";
        json << "    \"target\": \"" << r.target << "\",\n";
        json << "    \"duration\": " << r.durationSeconds << ",\n";
        json << "    \"status\": \"" << r.status << "\",\n";
        json << "    \"latency_avg\": " << r.avgLatencyMs << ",\n";
        json << "    \"packet_loss\": " << r.packetLossPc << "\n";
        json << "  }";
        if (i + 1 < records.size()) json << ",";
        json << "\n";
    }
    json << "]\n";

    return buildJsonResponse(200, json.str());
}

std::string RestApi::buildHtmlDashboard() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>NREPlatform Dashboard</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .card { background: white; border-radius: 8px; padding: 20px; margin: 10px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        h2 { color: #666; margin-top: 0; }
        .status { font-size: 24px; font-weight: bold; }
        .healthy { color: green; }
        .degraded { color: orange; }
        .critical { color: red; }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 10px; text-align: left; border-bottom: 1px solid #eee; }
        th { background: #f0f0f0; }
        .metric { display: inline-block; margin: 10px 20px 10px 0; }
        .metric-label { color: #888; font-size: 12px; }
        .metric-value { font-size: 20px; font-weight: bold; }
    </style>
</head>
<body>
    <h1>NREPlatform Dashboard</h1>

    <div class="card">
        <h2>Network Status</h2>
        <div id="status" class="status healthy">Loading...</div>
        <div id="metrics">
            <div class="metric">
                <div class="metric-label">Latency</div>
                <div class="metric-value" id="latency">-- ms</div>
            </div>
            <div class="metric">
                <div class="metric-label">Packet Loss</div>
                <div class="metric-value" id="loss">-- %</div>
            </div>
            <div class="metric">
                <div class="metric-label">Jitter</div>
                <div class="metric-value" id="jitter">-- ms</div>
            </div>
            <div class="metric">
                <div class="metric-label">Bandwidth</div>
                <div class="metric-value" id="bandwidth">-- Mb/s</div>
            </div>
        </div>
    </div>

    <div class="card">
        <h2>History</h2>
        <table>
            <thead>
                <tr><th>ID</th><th>Name</th><th>Type</th><th>Target</th><th>Duration</th><th>Status</th></tr>
            </thead>
            <tbody id="history">
                <tr><td colspan="6">Loading...</td></tr>
            </tbody>
        </table>
    </div>

    <script>
        fetch('/api/status').then(r => r.json()).then(data => {
            document.getElementById('status').textContent = data.status;
        });

        fetch('/api/history').then(r => r.json()).then(data => {
            const tbody = document.getElementById('history');
            if (data.length === 0) {
                tbody.innerHTML = '<tr><td colspan="6">No experiments yet.</td></tr>';
                return;
            }
            tbody.innerHTML = data.map(r =>
                '<tr><td>' + r.id + '</td><td>' + r.name + '</td><td>' + r.type +
                '</td><td>' + r.target + '</td><td>' + r.duration + 's</td><td>' +
                r.status + '</td></tr>'
            ).join('');
        });

        setInterval(() => {
            fetch('/api/status').then(r => r.json()).then(data => {
                document.getElementById('status').textContent = data.status;
            });
        }, 30000);
    </script>
</body>
</html>
)rawliteral";
}
