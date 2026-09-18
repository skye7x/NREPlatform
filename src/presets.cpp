#include "presets.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>

PresetManager::PresetManager() : customFolder("presets") {
    mkdir(customFolder.c_str(), 0755);
    initBuiltInPresets();
}

void PresetManager::initBuiltInPresets() {
    builtInPresets.clear();

    auto addPreset = [&](const std::string &name, const std::string &desc,
                          int lat, int jit, double loss, int dl, int ul, int dur) {
        PresetScenario p;
        p.name = name;
        p.description = desc;
        p.profile.name = desc;
        p.profile.latencyMs = lat;
        p.profile.jitterMs = jit;
        p.profile.packetLossPc = loss;
        p.profile.downloadKbit = dl;
        p.profile.uploadKbit = ul;
        p.profile.durationSeconds = dur;
        builtInPresets.push_back(p);
    };

    addPreset("weak_wifi", "Simulates weak Wi-Fi signal", 80, 25, 2.0, 0, 0, 60);
    addPreset("unstable_connection", "Simulates an unstable connection", 150, 80, 5.0, 0, 0, 60);
    addPreset("congested_network", "Simulates a congested network", 50, 10, 1.0, 2000, 500, 60);
    addPreset("poor_mobile", "Simulates poor mobile network", 120, 40, 3.0, 5000, 1000, 120);
    addPreset("satellite_network", "Simulates satellite network", 600, 30, 0.5, 10000, 2000, 120);
    addPreset("gaming", "Simulates gaming conditions", 50, 20, 1.0, 0, 0, 60);
    addPreset("voip", "Simulates VoIP call conditions", 30, 15, 0.5, 0, 0, 60);
    addPreset("streaming", "Simulates video streaming conditions", 40, 10, 0.2, 3000, 0, 120);
    addPreset("extreme_loss", "Simulates extreme packet loss", 20, 5, 25.0, 0, 0, 30);
}

std::vector<PresetScenario> PresetManager::getBuiltInPresets() {
    return builtInPresets;
}

PresetScenario PresetManager::getPreset(const std::string &name, bool &found) {
    for (const auto &preset : builtInPresets) {
        if (preset.name == name) { found = true; return preset; }
    }
    
    std::string path = customFolder + "/" + name + ".json";
    std::ifstream file(path);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        found = true;
        return jsonToPreset(content);
    }
    found = false;
    return PresetScenario();
}

std::vector<std::string> PresetManager::listPresetNames() {
    std::vector<std::string> names;
    for (const auto &p : builtInPresets) names.push_back(p.name);
    for (const auto &n : listCustomPresets()) names.push_back(n + " (custom)");
    return names;
}

bool PresetManager::saveAsProfile(const std::string &presetName, const std::string &profileName) {
    bool found = false;
    PresetScenario preset = getPreset(presetName, found);
    if (!found) return false;
    ExperimentProfile profile = preset.profile;
    profile.name = profileName;
    return true;
}

std::string PresetManager::presetToJson(const PresetScenario &preset) {
    std::ostringstream j;
    j << "{\n";
    j << "  \"name\": \"" << preset.name << "\",\n";
    j << "  \"description\": \"" << preset.description << "\",\n";
    j << "  \"profile\": {\n";
    j << "    \"name\": \"" << preset.profile.name << "\",\n";
    j << "    \"target_ip\": \"" << preset.profile.targetIp << "\",\n";
    j << "    \"target_mac\": \"" << preset.profile.targetMac << "\",\n";
    j << "    \"latency_ms\": " << preset.profile.latencyMs << ",\n";
    j << "    \"jitter_ms\": " << preset.profile.jitterMs << ",\n";
    j << "    \"packet_loss_pc\": " << preset.profile.packetLossPc << ",\n";
    j << "    \"download_kbit\": " << preset.profile.downloadKbit << ",\n";
    j << "    \"upload_kbit\": " << preset.profile.uploadKbit << ",\n";
    j << "    \"duration_seconds\": " << preset.profile.durationSeconds << "\n";
    j << "  }\n";
    j << "}\n";
    return j.str();
}

PresetScenario PresetManager::jsonToPreset(const std::string &json) {
    PresetScenario preset;
    std::istringstream iss(json);
    std::string line;
    while (std::getline(iss, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        auto trim = [](std::string &s) {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(0, 1);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == ',' || s.back() == '"')) s.pop_back();
            if (!s.empty() && s.front() == '"') s.erase(0, 1);
            if (!s.empty() && s.back() == '"') s.pop_back();
        };
        trim(key); trim(val);

        if (key == "name" && preset.name.empty()) preset.name = val;
        else if (key == "description") preset.description = val;
        else if (key == "name" && !preset.name.empty()) preset.profile.name = val;
        else if (key == "target_ip") preset.profile.targetIp = val;
        else if (key == "target_mac") preset.profile.targetMac = val;
        else if (key == "latency_ms") preset.profile.latencyMs = std::atoi(val.c_str());
        else if (key == "jitter_ms") preset.profile.jitterMs = std::atoi(val.c_str());
        else if (key == "packet_loss_pc") preset.profile.packetLossPc = std::atof(val.c_str());
        else if (key == "download_kbit") preset.profile.downloadKbit = std::atoi(val.c_str());
        else if (key == "upload_kbit") preset.profile.uploadKbit = std::atoi(val.c_str());
        else if (key == "duration_seconds") preset.profile.durationSeconds = std::atoi(val.c_str());
    }
    return preset;
}

bool PresetManager::exportPreset(const std::string &presetName, const std::string &outputPath) {
    bool found = false;
    PresetScenario preset = getPreset(presetName, found);
    if (!found) return false;

    std::ofstream file(outputPath);
    if (!file.is_open()) return false;
    file << presetToJson(preset);
    file.close();
    return true;
}

bool PresetManager::exportAllPresets(const std::string &outputPath) {
    std::ofstream file(outputPath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < builtInPresets.size(); ++i) {
        file << presetToJson(builtInPresets[i]);
        if (i + 1 < builtInPresets.size()) file << ",";
        file << "\n";
    }
    file << "]\n";
    file.close();
    return true;
}

bool PresetManager::importPreset(const std::string &inputPath) {
    std::ifstream file(inputPath);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    PresetScenario preset = jsonToPreset(content);
    if (preset.name.empty()) return false;

    return saveCustomPreset(preset);
}

bool PresetManager::importAllPresets(const std::string &inputPath) {
    std::ifstream file(inputPath);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::vector<std::string> objects;
    int depth = 0;
    std::string current;
    bool inArray = false;

    for (char c : content) {
        if (c == '[' && depth == 0) { inArray = true; continue; }
        if (c == ']' && depth == 0 && inArray) { break; }
        if (c == '{') depth++;
        if (c == '}') { depth--; if (depth == 0 && inArray) { objects.push_back(current); current.clear(); continue; } }
        if (depth > 0) current += c;
    }

    for (const auto &obj : objects) {
        PresetScenario preset = jsonToPreset("{" + obj + "}");
        if (!preset.name.empty()) saveCustomPreset(preset);
    }

    return true;
}

bool PresetManager::saveCustomPreset(const PresetScenario &preset) {
    std::string path = customFolder + "/" + preset.name + ".json";
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << presetToJson(preset);
    file.close();
    return true;
}

std::vector<std::string> PresetManager::listCustomPresets() {
    std::vector<std::string> names;
    DIR *dir = opendir(customFolder.c_str());
    if (!dir) return names;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.find(".json") != std::string::npos) {
            names.push_back(filename.substr(0, filename.size() - 5));
        }
    }
    closedir(dir);
    return names;
}
