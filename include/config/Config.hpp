#pragma once

#include "config/SaberConfig.hpp"
#include <string>
#include <utility>
#include <vector>

namespace FreeSabers {

class Config {
public:
    static Config& GetInstance();

    void Initialize(const std::string& modDataPath);

    // Directories
    const std::string& GetPresetDirectory() const { return presetDir; }
    const std::string& GetImportDirectory() const { return importDir; }

    // Presets
    const std::vector<FreeSaberConfig>& GetPresets() const { return presets; }
    const FreeSaberConfig* GetPreset(const std::string& name) const;
    const FreeSaberConfig* GetActiveLeftPreset() const;
    const FreeSaberConfig* GetActiveRightPreset() const;

    const std::string& GetActiveLeftName() const { return activeLeftPreset; }
    const std::string& GetActiveRightName() const { return activeRightPreset; }

    int GetPresetIndex(const std::string& name) const;
    bool SetActiveLeftPreset(const std::string& presetName);
    bool SetActiveRightPreset(const std::string& presetName);

    // Re-imports .reesaber files, reloads presets and fixes active names.
    void ReloadPresets();

    // Converts every .reesaber/.json preset found in the ReeSabers and Presets
    // folders into .freesaber files. Returns the number of imported presets.
    int ImportReeSabersFiles(bool overwrite = false);

    // Saves a copy of the active left preset using the next free name.
    bool SaveActivePresetCopy();

    int GetLastImportCount() const { return lastImportCount; }
    const std::string& GetLastImportStatus() const { return lastImportStatus; }

    // Global effect toggles
    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool value);

    bool AreTrailsEnabled() const { return trailsEnabled; }
    void SetTrailsEnabled(bool value);

    bool AreSparksEnabled() const { return sparksEnabled; }
    void SetSparksEnabled(bool value);

    bool IsGlowEnabled() const { return glowEnabled; }
    void SetGlowEnabled(bool value);

    // Performance settings
    int GetMaxTrailSamples() const { return maxTrailSamples; }
    void SetMaxTrailSamples(int value);

    int GetMaxSparkParticles() const { return maxSparkParticles; }
    void SetMaxSparkParticles(int value);

    bool IsPerformanceMode() const { return performanceMode; }
    void SetPerformanceMode(bool value);

    bool IsQuest() const { return isQuest; }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string modDataPath;
    std::string presetDir;
    std::string importDir;
    std::string settingsPath;
    std::string importLogPath;

    std::vector<FreeSaberConfig> presets;
    std::vector<std::pair<std::string, std::string>> importLog;

    std::string activeLeftPreset = "Default";
    std::string activeRightPreset = "Default";

    bool enabled = true;
    bool trailsEnabled = true;
    bool sparksEnabled = true;
    bool glowEnabled = true;

    // Performance settings
    int maxTrailSamples = 96;
    int maxSparkParticles = 160;
    bool performanceMode = false;
    bool isQuest = false;

    int lastImportCount = 0;
    std::string lastImportStatus;

    bool LoadPresets();
    void CreateDefaultPreset();
    void EnsureActivePresets();

    void LoadSettings();
    void SaveSettings();

    void LoadImportLog();
    void SaveImportLog();
    std::string ImportedNameFor(const std::string& sourcePath) const;
    bool ImportFile(const std::string& sourcePath, bool overwrite, std::string& outName);

    std::string UniquePresetName(const std::string& desired) const;
    static std::string SanitizeFileName(const std::string& desired);
};

} // namespace FreeSabers
