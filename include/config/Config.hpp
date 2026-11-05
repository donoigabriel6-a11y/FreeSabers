#pragma once

#include "config/SaberConfig.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace FreeSabers {

class Config {
public:
    static Config& GetInstance();

    // Initialize with mod info (call from setup)
    void Initialize(const std::string& modDataPath);

    // Load all presets from the presets directory
    void LoadPresets();

    // Save a preset to file
    bool SavePreset(const FreeSaberConfig& preset, const std::string& fileName);

    // Delete a preset file
    bool DeletePreset(const std::string& fileName);

    // Get all loaded presets
    const std::vector<FreeSaberConfig>& GetPresets() const { return presets; }

    // Get preset by name
    const FreeSaberConfig* GetPreset(const std::string& name) const;

    // Get current active preset for left/right saber
    const FreeSaberConfig* GetActiveLeftPreset() const;
    const FreeSaberConfig* GetActiveRightPreset() const;

    // Set active preset
    void SetActiveLeftPreset(const std::string& presetName);
    void SetActiveRightPreset(const std::string& presetName);

    // Get preset directory path
    const std::string& GetPresetDirectory() const { return presetDir; }

    // Create default preset if none exist
    void CreateDefaultPreset();

    // Reload all presets from disk
    void ReloadPresets();

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string modDataPath;
    std::string presetDir;
    std::vector<FreeSaberConfig> presets;
    std::string activeLeftPreset = "Default";
    std::string activeRightPreset = "Default";

    void LoadActivePresets();
    void SaveActivePresets();
    std::string GetActivePresetsFile() const;
};

} // namespace FreeSabers