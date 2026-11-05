#include "hooks/UIHooks.hpp"
#include "saber/SaberManager.hpp"
#include "config/Config.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "main.hpp"
#include "BSML/Shared/BSML/MainThreadScheduler.hpp"
#include "BSML/Shared/BSML/Components/Settings/DropdownListSetting.hpp"
#include "BSML/Shared/BSML/Components/Settings/ToggleSetting.hpp"
#include "BSML/Shared/BSML/Components/Settings/IncrementSetting.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/FlowCoordinator.hpp"

using namespace FreeSabers;
using namespace BSML;
using namespace HMUI;

namespace FreeSabers::UI {

static bool uiInitialized = false;
static std::vector<std::string> presetNamesCache;
static int leftPresetIndexCache = 0;
static int rightPresetIndexCache = 0;

void InstallUIHooks() {
    PaperLogger.info("Installing UI hooks...");

    // Hook into main menu flow coordinator to add our settings
    // This is typically done via BSML or by hooking MenuTransitionsHelper
    // For now, we'll use a simple approach with MainThreadScheduler

    uiInitialized = true;
    PaperLogger.info("UI hooks installed!");
}

void ShowFreeSabersSettings() {
    // This would open our custom settings flow
    // Implementation depends on BSML flow coordinator setup
}

void OnSettingsMenuOpened() {
    // Refresh preset list when settings opened
    presetNamesCache = GetPresetNames();
    leftPresetIndexCache = GetLeftPresetIndex();
    rightPresetIndexCache = GetRightPresetIndex();
}

std::vector<std::string> GetPresetNames() {
    std::vector<std::string> names;
    const auto& presets = Config::GetInstance().GetPresets();
    for (const auto& preset : presets) {
        names.push_back(preset.name);
    }
    if (names.empty()) {
        names.push_back("Default");
    }
    return names;
}

void SetLeftPreset(int index) {
    if (index >= 0 && index < static_cast<int>(presetNamesCache.size())) {
        SaberManager::GetInstance().SetLeftPreset(presetNamesCache[index]);
        leftPresetIndexCache = index;
    }
}

void SetRightPreset(int index) {
    if (index >= 0 && index < static_cast<int>(presetNamesCache.size())) {
        SaberManager::GetInstance().SetRightPreset(presetNamesCache[index]);
        rightPresetIndexCache = index;
    }
}

int GetLeftPresetIndex() {
    const FreeSaberConfig* activePreset = Config::GetInstance().GetActiveLeftPreset();
    if (!activePreset) return 0;
    const std::string& activeName = activePreset->name;
    for (size_t i = 0; i < presetNamesCache.size(); ++i) {
        if (presetNamesCache[i] == activeName) return static_cast<int>(i);
    }
    return 0;
}

int GetRightPresetIndex() {
    const FreeSaberConfig* activePreset = Config::GetInstance().GetActiveRightPreset();
    if (!activePreset) return 0;
    const std::string& activeName = activePreset->name;
    for (size_t i = 0; i < presetNamesCache.size(); ++i) {
        if (presetNamesCache[i] == activeName) return static_cast<int>(i);
    }
    return 0;
}

void ReloadPresets() {
    SaberManager::GetInstance().ReloadPresets();
    presetNamesCache = GetPresetNames();
    leftPresetIndexCache = GetLeftPresetIndex();
    rightPresetIndexCache = GetRightPresetIndex();
}

} // namespace FreeSabers::UI