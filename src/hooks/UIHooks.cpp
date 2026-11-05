#include "hooks/UIHooks.hpp"
#include "saber/SaberManager.hpp"
#include "config/Config.hpp"
#include "main.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Transform.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace FreeSabers::UI {

namespace {

std::string BuildStatusText() {
    auto& config = Config::GetInstance();
    std::string text = std::to_string(config.GetPresets().size()) + " preset(s)\nLeft: " +
        config.GetActiveLeftName() + "  |  Right: " + config.GetActiveRightName();

    if (!config.GetLastImportStatus().empty()) {
        text += "\n" + config.GetLastImportStatus();
    }
    text += "\nImport folder: " + config.GetImportDirectory();
    return text;
}

int PresetIndex(bool left) {
    auto& config = Config::GetInstance();
    return std::clamp(config.GetPresetIndex(left ? config.GetActiveLeftName() : config.GetActiveRightName()),
                      0, std::max(0, static_cast<int>(config.GetPresets().size()) - 1));
}

void SelectPreset(bool left, float value) {
    auto& config = Config::GetInstance();
    const auto& presets = config.GetPresets();
    if (presets.empty()) return;

    const int index = std::clamp(static_cast<int>(std::lround(value)), 0,
                                 static_cast<int>(presets.size()) - 1);
    if (left) {
        config.SetActiveLeftPreset(presets[index].name);
    } else {
        config.SetActiveRightPreset(presets[index].name);
    }
    SaberManager::GetInstance().ReapplyPresets();
}

} // namespace

void InstallUIHooks() {
    PaperLogger.info("FreeSabers: registering settings menu");
    BSML::Init();
    BSML::Register::RegisterSettingsMenu("FreeSabers",
        [](HMUI::ViewController* self, bool first, bool, bool) {
            UnityEngine::Transform* transform = self->get_transform();

            // Rebuild the page every activation so the preset list stays current.
            if (!first) {
                for (int i = transform->get_childCount() - 1; i >= 0; --i) {
                    UnityEngine::Object::Destroy(transform->GetChild(i)->get_gameObject());
                }
            }

            UnityEngine::Transform* parent = BSML::Lite::CreateScrollableSettingsContainer(transform)->get_transform();
            auto* status = BSML::Lite::CreateText(parent, "", 2.6f);
            auto refresh = [status]() {
                status->set_text(StringW(BuildStatusText()));
            };
            refresh();

            auto& config = Config::GetInstance();

            BSML::Lite::CreateToggle(parent, "Enable FreeSabers", config.IsEnabled(), [refresh](bool value) {
                Config::GetInstance().SetEnabled(value);
                SaberManager::GetInstance().ApplyVisibility();
                refresh();
            });

            BSML::Lite::CreateToggle(parent, "Motion-blur trails", config.AreTrailsEnabled(), [refresh](bool value) {
                Config::GetInstance().SetTrailsEnabled(value);
                SaberManager::GetInstance().ApplyVisibility();
                refresh();
            });

            BSML::Lite::CreateToggle(parent, "Cut sparks", config.AreSparksEnabled(), [refresh](bool value) {
                Config::GetInstance().SetSparksEnabled(value);
                SaberManager::GetInstance().ApplyVisibility();
                refresh();
            });

            BSML::Lite::CreateToggle(parent, "Blade glow", config.IsGlowEnabled(), [refresh](bool value) {
                Config::GetInstance().SetGlowEnabled(value);
                SaberManager::GetInstance().ApplyVisibility();
                refresh();
            });

            const int presetCount = std::max(1, static_cast<int>(config.GetPresets().size()));
            const float maxIndex = static_cast<float>(presetCount - 1);

            BSML::Lite::CreateIncrementSetting(parent, "Left preset", 0, 1.0f,
                static_cast<float>(PresetIndex(true)), 0.0f, maxIndex, [refresh](float value) {
                    SelectPreset(true, value);
                    refresh();
                });

            BSML::Lite::CreateIncrementSetting(parent, "Right preset", 0, 1.0f,
                static_cast<float>(PresetIndex(false)), 0.0f, maxIndex, [refresh](float value) {
                    SelectPreset(false, value);
                    refresh();
                });

            BSML::Lite::CreateUIButton(parent, "Reload presets", [refresh]() {
                Config::GetInstance().ReloadPresets();
                SaberManager::GetInstance().ReapplyPresets();
                refresh();
            });

            BSML::Lite::CreateUIButton(parent, "Import .reesaber files", [refresh]() {
                Config::GetInstance().ImportReeSabersFiles(true);
                Config::GetInstance().ReloadPresets();
                SaberManager::GetInstance().ReapplyPresets();
                refresh();
            });

            BSML::Lite::CreateUIButton(parent, "Save active preset copy", [refresh]() {
                Config::GetInstance().SaveActivePresetCopy();
                refresh();
            });

            BSML::Lite::CreateText(parent,
                "Drop ReeSabers .reesaber presets into the ReeSabers folder above, then press Import.",
                2.3f);

            BSML::Lite::CreateText(parent,
                "FreeSabers converts blades, trails, glow, sparks, drivers and colour overrides. "
                "Custom textures and binary mesh assets are approximated.",
                2.3f);
        }, false);
}

std::vector<std::string> GetPresetNames() {
    std::vector<std::string> names;
    for (const auto& preset : Config::GetInstance().GetPresets()) {
        names.push_back(preset.name);
    }
    if (names.empty()) names.push_back("Default");
    return names;
}

void ReloadPresets() {
    SaberManager::GetInstance().ReloadPresets();
}

} // namespace FreeSabers::UI
