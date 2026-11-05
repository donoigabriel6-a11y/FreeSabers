#pragma once

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include <string>
#include <vector>

namespace FreeSabers::UI {

// Initialize UI hooks
void InstallUIHooks();

// Settings menu
void ShowFreeSabersSettings();
void OnSettingsMenuOpened();

// Preset selection
std::vector<std::string> GetPresetNames();
void SetLeftPreset(int index);
void SetRightPreset(int index);
int GetLeftPresetIndex();
int GetRightPresetIndex();

// Reload presets
void ReloadPresets();

} // namespace FreeSabers::UI