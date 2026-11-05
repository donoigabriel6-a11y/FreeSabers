#include "saber/SaberManager.hpp"
#include "saber/SaberModel.hpp"
#include "saber/TrailRenderer.hpp"
#include "saber/GlowEffect.hpp"
#include "saber/SparkSystem.hpp"
#include "saber/DynamicJoints.hpp"
#include "config/Config.hpp"
#include "main.hpp"
#include <algorithm>

namespace FreeSabers {

SaberManager& SaberManager::GetInstance() {
    static SaberManager instance;
    return instance;
}

void SaberManager::Initialize() {
    PaperLogger.info("Initializing SaberManager");
    ReloadPresets();
}

void SaberManager::OnSaberCreated(void* saberController, bool isLeftSaber) {
    PaperLogger.info("Saber created: %s", isLeftSaber ? "Left" : "Right");

    auto& instance = sabers[saberController];
    instance.controller = saberController;
    instance.isLeft = isLeftSaber;
    instance.presetName = isLeftSaber ? leftPresetName : rightPresetName;

    const FreeSaberConfig* preset = Config::GetInstance().GetPreset(instance.presetName);
    if (preset) {
        InitializeSaber(instance, preset, isLeftSaber);
    } else {
        PaperLogger.warn("Preset not found: %s", instance.presetName.c_str());
    }
}

void SaberManager::OnSaberDestroyed(void* saberController) {
    auto it = sabers.find(saberController);
    if (it != sabers.end()) {
        PaperLogger.info("Saber destroyed: %s", it->second.isLeft ? "Left" : "Right");
        sabers.erase(it);
    }
}

void SaberManager::OnSaberUpdate(void* saberController) {
    auto it = sabers.find(saberController);
    if (it == sabers.end()) return;

    SaberInstance& instance = it->second;
    if (!instance.initialized) return;

    // Update model if exists
    if (instance.model) instance.model->Update(0.016f); // ~60fps
}

void SaberManager::UpdateSaber(void* saberController, bool isLeftSaber, float deltaTime) {
    auto it = sabers.find(saberController);
    if (it == sabers.end()) return;

    SaberInstance& instance = it->second;

    if (!instance.initialized) {
        const FreeSaberConfig* preset = Config::GetInstance().GetPreset(instance.presetName);
        if (preset) {
            InitializeSaber(instance, preset, isLeftSaber);
        }
        return;
    }

    // Update all components
    if (instance.model) instance.model->Update(deltaTime);
    if (instance.trail) instance.trail->Update(deltaTime);
    if (instance.glow) instance.glow->Update(deltaTime);
    if (instance.sparks) instance.sparks->Update(deltaTime);
    if (instance.dynamicJoints) instance.dynamicJoints->Update(deltaTime);

    // Update drivers
    const FreeSaberConfig* preset = Config::GetInstance().GetPreset(instance.presetName);
    if (preset) {
        // Update global drivers
        for (const auto& driver : preset->leftSaber.globalDrivers) {
            // TODO: Get actual gameplay values and apply drivers
        }

        // Update layer drivers
        for (const auto& layer : preset->leftSaber.layers) {
            for (const auto& driver : layer.drivers) {
                // TODO: Apply driver to material
            }
        }
    }
}

void SaberManager::OnNoteCut(void* noteController, void* saberController, bool isLeftSaber,
                             int noteType, int cutDirection, float swingRating) {
    auto it = sabers.find(saberController);
    if (it == sabers.end()) return;

    SaberInstance& instance = it->second;

    // Trigger sparks on note cut
    if (instance.sparks) {
        instance.sparks->EmitOnCut(noteType, cutDirection, swingRating);
    }

    // Trigger glow pulse
    if (instance.glow) {
        instance.glow->PulseOnCut(swingRating);
    }
}

void SaberManager::SetLeftPreset(const std::string& presetName) {
    leftPresetName = presetName;
    Config::GetInstance().SetActiveLeftPreset(presetName);

    // Apply to existing left sabers
    for (auto& [controller, instance] : sabers) {
        if (instance.isLeft) {
            const FreeSaberConfig* preset = Config::GetInstance().GetPreset(presetName);
            if (preset) {
                ApplyPreset(instance, preset, true);
            }
        }
    }
}

void SaberManager::SetRightPreset(const std::string& presetName) {
    rightPresetName = presetName;
    Config::GetInstance().SetActiveRightPreset(presetName);

    // Apply to existing right sabers
    for (auto& [controller, instance] : sabers) {
        if (!instance.isLeft) {
            const FreeSaberConfig* preset = Config::GetInstance().GetPreset(presetName);
            if (preset) {
                ApplyPreset(instance, preset, false);
            }
        }
    }
}

const FreeSaberConfig* SaberManager::GetLeftPreset() const {
    return Config::GetInstance().GetPreset(leftPresetName);
}

const FreeSaberConfig* SaberManager::GetRightPreset() const {
    return Config::GetInstance().GetPreset(rightPresetName);
}

void SaberManager::ReloadPresets() {
    Config::GetInstance().ReloadPresets();
    PaperLogger.info("Presets reloaded");
}

SaberModel* SaberManager::GetSaberModel(void* saberController) {
    auto it = sabers.find(saberController);
    if (it != sabers.end()) {
        return it->second.model.get();
    }
    return nullptr;
}

void SaberManager::Cleanup() {
    sabers.clear();
    PaperLogger.info("SaberManager cleaned up");
}

void SaberManager::InitializeSaber(SaberInstance& instance, const FreeSaberConfig* preset, bool isLeft) {
    const SaberPreset& saberPreset = isLeft ? preset->leftSaber : preset->rightSaber;

    // Create components based on layers
    for (const auto& layer : saberPreset.layers) {
        if (!layer.enabled) continue;

        switch (layer.type) {
            case LayerType::Blade:
                if (!instance.model) {
                    instance.model = std::make_unique<SaberModel>();
                    instance.model->Initialize(instance.controller, layer, isLeft);
                }
                break;
            case LayerType::Trail:
                if (!instance.trail) {
                    instance.trail = std::make_unique<TrailRenderer>();
                    instance.trail->Initialize(instance.controller, layer.trail);
                }
                break;
            case LayerType::Glow:
                if (!instance.glow) {
                    instance.glow = std::make_unique<GlowEffect>();
                    instance.glow->Initialize(instance.controller, layer.glow);
                }
                break;
            case LayerType::Sparks:
                if (!instance.sparks) {
                    instance.sparks = std::make_unique<SparkSystem>();
                    instance.sparks->Initialize(instance.controller, layer.sparks);
                }
                break;
            case LayerType::DynamicJoints:
                if (!instance.dynamicJoints) {
                    instance.dynamicJoints = std::make_unique<DynamicJoints>();
                    instance.dynamicJoints->Initialize(instance.controller, layer.dynamicJoints);
                }
                break;
            case LayerType::Hilt:
            case LayerType::Custom:
                // Not implemented yet
                break;
        }
    }

    instance.initialized = true;
    instance.presetName = isLeft ? leftPresetName : rightPresetName;
    PaperLogger.info("Initialized saber: %s with %zu layers",
                     isLeft ? "Left" : "Right", saberPreset.layers.size());
}

void SaberManager::ApplyPreset(SaberInstance& instance, const FreeSaberConfig* preset, bool isLeft) {
    // Clean up existing components
    instance.model.reset();
    instance.trail.reset();
    instance.glow.reset();
    instance.sparks.reset();
    instance.dynamicJoints.reset();
    instance.initialized = false;

    // Re-initialize with new preset
    InitializeSaber(instance, preset, isLeft);
}

} // namespace FreeSabers