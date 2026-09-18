#ifndef PRESETS_H
#define PRESETS_H

#include <string>
#include <vector>
#include "profile.h"

struct PresetScenario {
    std::string name;
    std::string description;
    ExperimentProfile profile;
};

class PresetManager {
public:
    PresetManager();

    std::vector<PresetScenario> getBuiltInPresets();

    PresetScenario getPreset(const std::string &name, bool &found);

    std::vector<std::string> listPresetNames();

    bool saveAsProfile(const std::string &presetName, const std::string &profileName);

    bool exportPreset(const std::string &presetName, const std::string &outputPath);

    bool exportAllPresets(const std::string &outputPath);

    bool importPreset(const std::string &inputPath);

    bool importAllPresets(const std::string &inputPath);

    bool saveCustomPreset(const PresetScenario &preset);

    std::vector<std::string> listCustomPresets();

private:
    std::vector<PresetScenario> builtInPresets;
    std::string customFolder;
    void initBuiltInPresets();

    std::string presetToJson(const PresetScenario &preset);
    PresetScenario jsonToPreset(const std::string &json);
};

#endif
