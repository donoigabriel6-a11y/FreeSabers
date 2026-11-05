#include "config/Config.hpp"
#include "main.hpp"
#include <filesystem>
#include <fstream>

namespace FreeSabers {

static const char* kModId = "FreeSabers";

Config& Config::GetInstance() {
    static Config instance;
    return instance;
}

void Config::Initialize(const std::string& modDataPath) {
    this->modDataPath = modDataPath;
    presetDir = modDataPath + "/Presets";

    // Create directories if they don't exist
    std::filesystem::create_directories(modDataPath);
    std::filesystem::create_directories(presetDir);

    LoadPresets();
    LoadActivePresets();

    // Create default preset if none exist
    if (presets.empty()) {
        CreateDefaultPreset();
    }

    PaperLogger.info("Config initialized. Preset directory: %s", presetDir.c_str());
    PaperLogger.info("Loaded %zu presets", presets.size());
}

void Config::LoadPresets() {
    presets.clear();

    try {
        for (const auto& entry : std::filesystem::directory_iterator(presetDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".freesaber") {
                auto preset = FreeSaberConfig::LoadFromFile(entry.path().string());
                if (preset.has_value()) {
                    presets.push_back(std::move(preset.value()));
                    PaperLogger.info("Loaded preset: %s", entry.path().filename().string().c_str());
                } else {
                    PaperLogger.warn("Failed to load preset: %s", entry.path().filename().string().c_str());
                }
            }
        }
    } catch (const std::exception& e) {
        PaperLogger.error("Error loading presets: %s", e.what());
    }

    // Sort presets by name
    std::sort(presets.begin(), presets.end(), [](const FreeSaberConfig& a, const FreeSaberConfig& b) {
        return a.name < b.name;
    });
}

bool Config::SavePreset(const FreeSaberConfig& preset, const std::string& fileName) {
    std::string filePath = presetDir + "/" + fileName;
    if (fileName.find(".freesaber") == std::string::npos) {
        filePath += ".freesaber";
    }

    bool result = preset.SaveToFile(filePath);
    if (result) {
        PaperLogger.info("Saved preset: %s", filePath.c_str());
        LoadPresets(); // Reload to update list
    }
    return result;
}

bool Config::DeletePreset(const std::string& fileName) {
    std::string filePath = presetDir + "/" + fileName;
    if (std::filesystem::exists(filePath)) {
        bool result = std::filesystem::remove(filePath);
        if (result) {
            PaperLogger.info("Deleted preset: %s", filePath.c_str());
            LoadPresets(); // Reload to update list
        }
        return result;
    }
    return false;
}

const FreeSaberConfig* Config::GetPreset(const std::string& name) const {
    for (const auto& preset : presets) {
        if (preset.name == name) {
            return &preset;
        }
    }
    return nullptr;
}

const FreeSaberConfig* Config::GetActiveLeftPreset() const {
    return GetPreset(activeLeftPreset);
}

const FreeSaberConfig* Config::GetActiveRightPreset() const {
    if (activeRightPreset == activeLeftPreset) {
        return GetActiveLeftPreset();
    }
    return GetPreset(activeRightPreset);
}

void Config::SetActiveLeftPreset(const std::string& presetName) {
    if (GetPreset(presetName)) {
        activeLeftPreset = presetName;
        SaveActivePresets();
        PaperLogger.info("Set active left preset: %s", presetName.c_str());
    }
}

void Config::SetActiveRightPreset(const std::string& presetName) {
    if (GetPreset(presetName)) {
        activeRightPreset = presetName;
        SaveActivePresets();
        PaperLogger.info("Set active right preset: %s", presetName.c_str());
    }
}

void Config::CreateDefaultPreset() {
    FreeSaberConfig defaultPreset;
    defaultPreset.name = "Default";
    defaultPreset.author = "FreeSabers";
    defaultPreset.description = "Default FreeSabers preset";
    defaultPreset.useSameForBoth = true;

    // Create a basic blade layer for left saber
    LayerConfig bladeLayer;
    bladeLayer.name = "Main Blade";
    bladeLayer.type = LayerType::Blade;
    bladeLayer.enabled = true;
    bladeLayer.order = 0;
    bladeLayer.baseColor = Color(0.0f, 0.8f, 1.0f, 1.0f); // Cyan
    bladeLayer.emissionColor = Color(0.0f, 0.8f, 1.0f, 1.0f);
    bladeLayer.emissionIntensity = 2.0f;

    // Add basic trail
    bladeLayer.trail.enabled = true;
    bladeLayer.trail.width = 0.1f;
    bladeLayer.trail.length = 2.0f;
    bladeLayer.trail.lifetime = 0.3f;
    bladeLayer.trail.colorGradient.keys = {
        {0.0f, Color(0.0f, 0.8f, 1.0f, 1.0f)},
        {1.0f, Color(0.0f, 0.4f, 0.5f, 0.0f)}
    };

    // Add basic glow
    bladeLayer.glow.enabled = true;
    bladeLayer.glow.intensity = 1.5f;
    bladeLayer.glow.radius = 0.4f;
    bladeLayer.glow.color = Color(0.0f, 0.8f, 1.0f, 1.0f);

    // Add basic sparks
    bladeLayer.sparks.enabled = true;
    bladeLayer.sparks.count = 8;
    bladeLayer.sparks.size = 0.04f;
    bladeLayer.sparks.lifetime = 0.25f;
    bladeLayer.sparks.velocity = 1.5f;
    bladeLayer.sparks.spread = 30.0f;
    bladeLayer.sparks.colorGradient.keys = {
        {0.0f, Color(1.0f, 1.0f, 0.5f, 1.0f)},
        {1.0f, Color(1.0f, 0.5f, 0.0f, 0.0f)}
    };

    // Add score driver for emission intensity
    MaterialDriver scoreDriver;
    scoreDriver.name = "Score Emission";
    scoreDriver.type = DriverType::Score;
    scoreDriver.targetProperty = "_EmissionIntensity";
    scoreDriver.minValue = 0;
    scoreDriver.maxValue = 1000000;
    scoreDriver.multiplier = 1.0f;
    scoreDriver.offset = 1.0f;
    bladeLayer.drivers.push_back(scoreDriver);

    defaultPreset.leftSaber.layers.push_back(bladeLayer);
    defaultPreset.leftSaber.name = "Default Left";

    // Copy to right saber (will mirror if useSameForBoth)
    defaultPreset.rightSaber = defaultPreset.leftSaber;
    defaultPreset.rightSaber.name = "Default Right";

    // Save default preset
    SavePreset(defaultPreset, "Default.freesaber");
    PaperLogger.info("Created default preset");
}

void Config::ReloadPresets() {
    LoadPresets();
    LoadActivePresets();
}

void Config::LoadActivePresets() {
    std::string filePath = GetActivePresetsFile();
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string left, right;
        std::getline(file, left);
        std::getline(file, right);
        if (!left.empty()) activeLeftPreset = left;
        if (!right.empty()) activeRightPreset = right;
    }
}

void Config::SaveActivePresets() {
    std::string filePath = GetActivePresetsFile();
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << activeLeftPreset << "\n" << activeRightPreset << "\n";
    }
}

std::string Config::GetActivePresetsFile() const {
    return modDataPath + "/active_presets.txt";
}

} // namespace FreeSabers