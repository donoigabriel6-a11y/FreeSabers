#pragma once

#include "config/SaberConfig.hpp"
#include <string>
#include <vector>

namespace FreeSabers::ReeSabers {

struct ImportResult {
    bool success = false;
    std::string error;
    FreeSaberConfig config;
    std::vector<std::string> warnings;

    std::string Summary() const;
};

// Converts a .reesaber JSON document into a FreeSabers config. Supports the
// real 0.3.x module format (reezonate.blur-saber v1/v2, simple-trail,
// sparks-vfx, vanilla-saber, empty) including nested module children.
ImportResult Import(const std::string& json, const std::string& fallbackName);

// Cheap structural check used before attempting an import of a .json file.
bool LooksLikeReeSabers(const std::string& json);

} // namespace FreeSabers::ReeSabers
