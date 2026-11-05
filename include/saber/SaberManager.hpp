#pragma once

#include "config/SaberConfig.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace FreeSabers {

class SaberModel;
class TrailRenderer;
class GlowEffect;
class SparkSystem;
class DynamicJoints;

class SaberManager {
public:
    static SaberManager& GetInstance();

    // Initialize the manager
    void Initialize();

    // Called when a saber is created/updated
    void OnSaberCreated(void* saberController, bool isLeftSaber);
    void OnSaberDestroyed(void* saberController);

    // Called every frame for each saber
    void UpdateSaber(void* saberController, bool isLeftSaber, float deltaTime);

    // Called every frame for saber model updates
    void OnSaberUpdate(void* saberController);

    // Called when a note is cut
    void OnNoteCut(void* noteController, void* saberController, bool isLeftSaber,
                   int noteType, int cutDirection, float swingRating);

    // Set active preset for left/right saber
    void SetLeftPreset(const std::string& presetName);
    void SetRightPreset(const std::string& presetName);

    // Get current preset for a saber
    const FreeSaberConfig* GetLeftPreset() const;
    const FreeSaberConfig* GetRightPreset() const;

    // Reload presets
    void ReloadPresets();

    // Get or create saber model for a controller
    SaberModel* GetSaberModel(void* saberController);

    // Cleanup
    void Cleanup();

private:
    SaberManager() = default;
    ~SaberManager() = default;
    SaberManager(const SaberManager&) = delete;
    SaberManager& operator=(const SaberManager&) = delete;

    struct SaberInstance {
        void* controller = nullptr;
        std::unique_ptr<SaberModel> model;
        std::unique_ptr<TrailRenderer> trail;
        std::unique_ptr<GlowEffect> glow;
        std::unique_ptr<SparkSystem> sparks;
        std::unique_ptr<DynamicJoints> dynamicJoints;
        bool isLeft = false;
        std::string presetName;
        bool initialized = false;
    };

    std::unordered_map<void*, SaberInstance> sabers;
    std::string leftPresetName = "Default";
    std::string rightPresetName = "Default";

    void InitializeSaber(SaberInstance& instance, const FreeSaberConfig* preset, bool isLeft);
    void ApplyPreset(SaberInstance& instance, const FreeSaberConfig* preset, bool isLeft);
};

} // namespace FreeSabers