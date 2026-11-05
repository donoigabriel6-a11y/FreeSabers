#pragma once

#include <string>
#include <vector>

namespace FreeSabers::UI {

void InstallUIHooks();
std::vector<std::string> GetPresetNames();
void ReloadPresets();

} // namespace FreeSabers::UI
