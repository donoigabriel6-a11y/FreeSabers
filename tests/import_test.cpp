// Standalone importer test. It only depends on the config/import translation
// units (rapidjson + the C++ standard library), so it can run on a desktop
// toolchain without the Beat Saber headers:
//
//   g++ -std=c++20 -I include -I extern/includes/beatsaber-hook/shared/rapidjson/include \
//       tests/import_test.cpp src/config/SaberConfig.cpp src/config/JsonHelpers.cpp \
//       src/config/ReeSabersImport.cpp -o build/import_test
//
// Pass one or more .reesaber/.json files (optionally followed by an output
// folder) to check them.
#include "config/ReeSabersImport.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int Fail(const std::string& message) {
    std::cerr << "FAIL: " << message << "\n";
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: import_test <preset.reesaber|preset.json> [output-dir]\n";
        return 2;
    }

    std::vector<std::string> inputs;
    std::string outputDir;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (i == argc - 1 && std::filesystem::is_directory(argument)) {
            outputDir = argument;
        } else {
            inputs.push_back(argument);
        }
    }

    int failures = 0;
    for (const auto& input : inputs) {
        const std::string content = ReadFile(input);
        if (content.empty()) {
            failures += Fail(input + ": could not read file");
            continue;
        }

        const std::string stem = std::filesystem::path(input).stem().string();
        auto result = FreeSabers::ReeSabers::Import(content, stem);
        if (!result.success) {
            failures += Fail(input + ": " + result.error);
            continue;
        }

        const auto& preset = result.config.GetPreset(true);
        std::cout << "PASS " << input << "\n"
                  << "  name: " << result.config.name << "\n"
                  << "  source: " << result.config.sourceFormat << " " << result.config.sourceVersion << "\n"
                  << "  blade: " << (preset.blade.enabled ? "yes" : "no")
                  << " radius=" << preset.blade.radius
                  << " profile=" << preset.blade.profile.points.size()
                  << " colours=" << preset.blade.colorOverValue.points.size() << "\n"
                  << "  trails: " << preset.trails.size()
                  << "  sparks: " << (preset.sparks.enabled ? "yes" : "no")
                  << " (" << preset.sparks.count << ")"
                  << "  drivers: " << preset.drivers.size() << "\n";
        if (!result.warnings.empty()) {
            for (const auto& warning : result.warnings) {
                std::cout << "  warning: " << warning << "\n";
            }
        }
        if (preset.blade.enabled && preset.blade.profile.points.size() < 2) {
            failures += Fail(input + ": blade profile was not imported");
        }
        if (preset.trails.empty()) {
            failures += Fail(input + ": no trails were imported");
        }

        const std::string json = result.config.ToJsonString(true);
        FreeSabers::FreeSaberConfig roundTrip;
        if (!roundTrip.FromJsonString(json)) {
            failures += Fail(input + ": .freesaber round-trip parse failed");
            continue;
        }
        const auto& roundTripped = roundTrip.GetPreset(true);
        if (roundTripped.trails.size() != preset.trails.size()) {
            failures += Fail(input + ": trail count changed during round-trip");
        }
        if (std::fabs(roundTripped.blade.radius - preset.blade.radius) > 1e-5f) {
            failures += Fail(input + ": blade radius changed during round-trip");
        }
        if (roundTripped.sparks.enabled != preset.sparks.enabled) {
            failures += Fail(input + ": sparks enabled state changed during round-trip");
        }

        if (!outputDir.empty()) {
            const std::string output = outputDir + "/" + stem + ".freesaber";
            if (!result.config.SaveToFile(output)) {
                failures += Fail(input + ": could not write " + output);
            } else {
                std::cout << "  wrote: " << output << "\n";
            }
        }
    }

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed (" << inputs.size() << " preset(s))\n";
    return 0;
}
