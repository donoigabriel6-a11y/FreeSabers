#include "config/Config.hpp"
#include "config/JsonHelpers.hpp"
#include "config/ReeSabersImport.hpp"
#include "main.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <sstream>

namespace FreeSabers {

namespace {

namespace fs = std::filesystem;

bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool EndsWith(const std::string& value, const std::string& suffix) {
    if (suffix.size() > value.size()) return false;
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string Trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

} // namespace

Config& Config::GetInstance() {
    static Config instance;
    return instance;
}

void Config::Initialize(const std::string& dataPath) {
    modDataPath = dataPath;
    presetDir = modDataPath + "/Presets";
    importDir = modDataPath + "/ReeSabers";
    settingsPath = modDataPath + "/settings.json";
    importLogPath = modDataPath + "/imported.txt";

    std::error_code error;
    fs::create_directories(presetDir, error);
    fs::create_directories(importDir, error);

    LoadImportLog();
    LoadSettings();

    // Convert any ReeSabers presets dropped into the import folder before the
    // .freesaber scan so they show up immediately.
    ImportReeSabersFiles(false);

    if (!LoadPresets() || presets.empty()) {
        CreateDefaultPreset();
    }
    EnsureActivePresets();

    PaperLogger.info("FreeSabers: loaded %d preset(s) from %s",
                     static_cast<int>(presets.size()), presetDir.c_str());
}

bool Config::LoadPresets() {
    presets.clear();

    std::error_code error;
    if (!fs::exists(presetDir, error)) return false;

    for (const auto& entry : fs::directory_iterator(presetDir, error)) {
        if (error) break;
        if (!entry.is_regular_file()) continue;
        if (Lower(entry.path().extension().string()) != ".freesaber") continue;

        auto preset = FreeSaberConfig::LoadFromFile(entry.path().string());
        if (!preset) {
            PaperLogger.error("FreeSabers: failed to load preset %s",
                              entry.path().filename().string().c_str());
            continue;
        }
        presets.push_back(std::move(*preset));
    }

    std::sort(presets.begin(), presets.end(), [](const FreeSaberConfig& a, const FreeSaberConfig& b) {
        return Lower(a.name) < Lower(b.name);
    });
    return true;
}

void Config::CreateDefaultPreset() {
    FreeSaberConfig config;
    config.name = "Default";
    config.author = "FreeSabers";
    config.description = "Built-in FreeSabers preset";
    config.useSameForBoth = true;

    SaberPreset preset;
    preset.name = "Default";
    preset.author = "FreeSabers";
    preset.description = "Cyan blade with a motion-blur trail";

    BladeConfig& blade = preset.blade;
    blade.enabled = true;
    blade.radius = 0.014f;
    blade.blurFrames = 2.5f;
    blade.blurSensitivity = 0.7f;
    blade.baseColor = Color(0.1f, 0.8f, 1.0f, 1.0f);
    blade.emissionColor = Color(0.1f, 0.8f, 1.0f, 1.0f);
    blade.envLightColor = Color(0.1f, 0.8f, 1.0f, 1.0f);
    blade.emissionIntensity = 2.5f;
    blade.profile.points = {
        {0.0f, 0.35f},
        {0.08f, 1.0f},
        {0.85f, 0.9f},
        {1.0f, 0.25f},
    };
    blade.alphaOverValue.points = {{0.0f, 1.0f}, {1.0f, 1.0f}};
    blade.colorOverValue.points = {
        {0.0f, Color(0.03f, 0.35f, 0.9f, 1.0f)},
        {0.5f, Color(0.1f, 0.85f, 1.0f, 1.0f)},
        {1.0f, Color(0.75f, 1.0f, 1.0f, 1.0f)},
    };

    TrailConfig bladeTrail;
    bladeTrail.name = "Blade Trail";
    bladeTrail.trailType = 1;
    bladeTrail.length = 0.22f;
    bladeTrail.width = 0.03f;
    bladeTrail.speedWidth = 0.05f;
    bladeTrail.speedAlpha = 0.35f;
    bladeTrail.colorOverAge.points = {
        {0.0f, Color(0.4f, 0.95f, 1.0f, 1.0f)},
        {1.0f, Color(0.0f, 0.25f, 0.6f, 0.0f)},
    };
    bladeTrail.alphaOverAge.points = {{0.0f, 1.0f}, {1.0f, 0.0f}};
    bladeTrail.widthOverAge.points = {{0.0f, 1.0f}, {1.0f, 0.15f}};
    preset.trails.push_back(bladeTrail);

    TrailConfig tipTrail;
    tipTrail.name = "Tip Trail";
    tipTrail.trailType = 0;
    tipTrail.length = 0.16f;
    tipTrail.width = 0.035f;
    tipTrail.speedWidth = 0.04f;
    tipTrail.colorOverAge.points = {
        {0.0f, Color(1.0f, 1.0f, 1.0f, 1.0f)},
        {1.0f, Color(0.1f, 0.6f, 1.0f, 0.0f)},
    };
    tipTrail.alphaOverAge.points = {{0.0f, 1.0f}, {1.0f, 0.0f}};
    preset.trails.push_back(tipTrail);

    preset.sparks.enabled = true;
    preset.sparks.count = 96;
    preset.sparks.size = 0.03f;
    preset.sparks.minLifetime = 0.2f;
    preset.sparks.maxLifetime = 0.5f;
    preset.sparks.velocity = 1.8f;
    preset.sparks.spread = 40.0f;
    preset.sparks.damping = 1.2f;
    preset.sparks.color = Color(0.6f, 0.95f, 1.0f, 1.0f);
    preset.sparks.colorOverLifetime.points = {
        {0.0f, Color(1.0f, 1.0f, 1.0f, 1.0f)},
        {0.4f, Color(0.3f, 0.9f, 1.0f, 1.0f)},
        {1.0f, Color(0.0f, 0.3f, 0.8f, 1.0f)},
    };
    preset.sparks.alphaOverLifetime.points = {{0.0f, 1.0f}, {1.0f, 0.0f}};

    preset.glow.enabled = true;
    preset.glow.intensity = 1.8f;
    preset.glow.radius = 0.55f;
    preset.glow.color = Color(0.1f, 0.8f, 1.0f, 1.0f);

    config.leftSaber = preset;
    config.rightSaber = preset;

    std::string error;
    if (!config.Validate(error)) {
        PaperLogger.error("FreeSabers: default preset is invalid: %s", error.c_str());
    }
    if (!config.SaveToFile(presetDir + "/Default.freesaber")) {
        PaperLogger.error("FreeSabers: failed to write the default preset");
    }
    LoadPresets();
}

const FreeSaberConfig* Config::GetPreset(const std::string& name) const {
    for (const auto& preset : presets) {
        if (preset.name == name) return &preset;
    }
    return nullptr;
}

const FreeSaberConfig* Config::GetActiveLeftPreset() const {
    if (const auto* preset = GetPreset(activeLeftPreset)) return preset;
    return presets.empty() ? nullptr : &presets.front();
}

const FreeSaberConfig* Config::GetActiveRightPreset() const {
    if (const auto* preset = GetPreset(activeRightPreset)) return preset;
    return presets.empty() ? nullptr : &presets.front();
}

int Config::GetPresetIndex(const std::string& name) const {
    for (size_t i = 0; i < presets.size(); ++i) {
        if (presets[i].name == name) return static_cast<int>(i);
    }
    return 0;
}

bool Config::SetActiveLeftPreset(const std::string& presetName) {
    if (!GetPreset(presetName)) return false;
    activeLeftPreset = presetName;
    SaveSettings();
    PaperLogger.info("FreeSabers: active left preset = %s", presetName.c_str());
    return true;
}

bool Config::SetActiveRightPreset(const std::string& presetName) {
    if (!GetPreset(presetName)) return false;
    activeRightPreset = presetName;
    SaveSettings();
    PaperLogger.info("FreeSabers: active right preset = %s", presetName.c_str());
    return true;
}

void Config::EnsureActivePresets() {
    if (presets.empty()) return;
    bool changed = false;
    if (!GetPreset(activeLeftPreset)) {
        activeLeftPreset = presets.front().name;
        changed = true;
    }
    if (!GetPreset(activeRightPreset)) {
        activeRightPreset = presets.front().name;
        changed = true;
    }
    if (changed) SaveSettings();
}

void Config::ReloadPresets() {
    const int imported = ImportReeSabersFiles(false);
    LoadPresets();
    if (presets.empty()) CreateDefaultPreset();
    EnsureActivePresets();

    lastImportCount = imported;
    if (imported > 0) {
        lastImportStatus = "Imported " + std::to_string(imported) + " ReeSabers preset(s)";
    }
    PaperLogger.info("FreeSabers: reloaded %d preset(s)", static_cast<int>(presets.size()));
}

std::string Config::SanitizeFileName(const std::string& desired) {
    std::string result;
    result.reserve(desired.size());
    for (unsigned char c : desired) {
        if (std::isalnum(c) || c == ' ' || c == '-' || c == '_' || c == '(' || c == ')') {
            result.push_back(static_cast<char>(c));
        } else {
            result.push_back('_');
        }
    }
    result = Trim(result);
    while (!result.empty() && (result.back() == '.' || result.back() == ' ')) result.pop_back();
    if (result.size() > 64) result.resize(64);
    if (result.empty()) result = "Preset";
    return result;
}

std::string Config::UniquePresetName(const std::string& desired) const {
    const std::string base = desired.empty() ? "Preset" : Trim(desired);

    auto taken = [this](const std::string& candidate) {
        const std::string lower = Lower(candidate);
        for (const auto& preset : presets) {
            if (Lower(preset.name) == lower) return true;
        }
        std::error_code error;
        if (fs::exists(presetDir, error)) {
            for (const auto& entry : fs::directory_iterator(presetDir, error)) {
                if (error) break;
                if (!entry.is_regular_file()) continue;
                if (Lower(entry.path().stem().string()) == lower) return true;
            }
        }
        return false;
    };

    std::string candidate = base;
    int suffix = 2;
    while (taken(candidate)) {
        candidate = base + " (" + std::to_string(suffix) + ")";
        ++suffix;
        if (suffix > 999) break;
    }
    return candidate;
}

std::string Config::ImportedNameFor(const std::string& sourcePath) const {
    const std::string source = fs::path(sourcePath).generic_string();
    for (const auto& [path, name] : importLog) {
        if (path == source) return name;
    }
    // Tolerate a changed ModData prefix by matching the file name.
    const std::string fileName = fs::path(sourcePath).filename().string();
    for (const auto& [path, name] : importLog) {
        if (fs::path(path).filename().string() == fileName) return name;
    }
    return {};
}

bool Config::ImportFile(const std::string& sourcePath, bool overwrite, std::string& outName) {
    std::string content;
    if (!ReadFile(sourcePath, content)) {
        PaperLogger.error("FreeSabers: cannot read %s", sourcePath.c_str());
        return false;
    }
    if (content.size() > 4 * 1024 * 1024) {
        PaperLogger.error("FreeSabers: %s is larger than 4 MB and was skipped", sourcePath.c_str());
        return false;
    }

    if (Lower(fs::path(sourcePath).extension().string()) == ".json" &&
        !ReeSabers::LooksLikeReeSabers(content)) {
        return false;
    }

    const std::string stem = fs::path(sourcePath).stem().string();
    auto result = ReeSabers::Import(content, stem.empty() ? "ReeSabers Preset" : stem);
    if (!result.success) {
        PaperLogger.error("FreeSabers: %s: %s", sourcePath.c_str(), result.error.c_str());
        return false;
    }

    const std::string previous = ImportedNameFor(sourcePath);
    const std::string name = (overwrite && !previous.empty()) ? previous : UniquePresetName(stem);

    FreeSaberConfig config = std::move(result.config);
    config.name = name;
    config.author = "ReeSabers";
    config.leftSaber.name = name;
    config.rightSaber.name = name;

    if (!result.warnings.empty()) {
        config.description += " (" + std::to_string(result.warnings.size()) + " import warning(s))";
        for (const auto& warning : result.warnings) {
            PaperLogger.warn("FreeSabers import: %s", warning.c_str());
        }
    }

    const std::string fileName = SanitizeFileName(name) + ".freesaber";
    if (!config.SaveToFile(presetDir + "/" + fileName)) {
        PaperLogger.error("FreeSabers: failed to write %s", fileName.c_str());
        return false;
    }

    bool updated = false;
    for (auto& [source, output] : importLog) {
        if (source == fs::path(sourcePath).generic_string()) {
            output = name;
            updated = true;
            break;
        }
    }
    if (!updated) importLog.emplace_back(fs::path(sourcePath).generic_string(), name);

    outName = name;
    PaperLogger.info("FreeSabers: imported %s -> %s", sourcePath.c_str(), fileName.c_str());
    return true;
}

int Config::ImportReeSabersFiles(bool overwrite) {
    std::vector<std::string> sources;

    auto collect = [&sources](const std::string& directory) {
        std::error_code error;
        if (!fs::exists(directory, error)) return;
        for (const auto& entry : fs::directory_iterator(directory, error)) {
            if (error) break;
            if (!entry.is_regular_file()) continue;
            const std::string extension = Lower(entry.path().extension().string());
            if (extension != ".reesaber" && extension != ".json") continue;
            sources.push_back(entry.path().string());
        }
    };
    collect(importDir);
    collect(presetDir);

    int imported = 0;
    int failed = 0;
    for (const auto& source : sources) {
        if (!overwrite) {
            const std::string existing = ImportedNameFor(source);
            if (!existing.empty() &&
                fs::exists(presetDir + "/" + SanitizeFileName(existing) + ".freesaber")) {
                continue;
            }
        }
        std::string name;
        if (ImportFile(source, overwrite, name)) {
            ++imported;
        } else {
            ++failed;
        }
    }

    SaveImportLog();

    lastImportCount = imported;
    if (imported > 0) {
        lastImportStatus = "Imported " + std::to_string(imported) + " ReeSabers preset(s)";
    } else if (failed > 0) {
        lastImportStatus = "ReeSabers import failed for " + std::to_string(failed) + " file(s)";
    } else {
        lastImportStatus = "No new .reesaber files found";
    }

    if (imported > 0 || failed > 0) {
        PaperLogger.info("FreeSabers: ReeSabers import finished (%d ok, %d failed)", imported, failed);
    }
    return imported;
}

bool Config::SaveActivePresetCopy() {
    const FreeSaberConfig* active = GetActiveLeftPreset();
    if (!active) return false;

    FreeSaberConfig copy = *active;
    const std::string baseName = copy.name.empty() ? "Preset" : copy.name;
    const std::string name = UniquePresetName(baseName + " Copy");
    copy.name = name;
    copy.author = "FreeSabers";
    copy.leftSaber.name = name;
    copy.rightSaber.name = name;

    if (!copy.SaveToFile(presetDir + "/" + SanitizeFileName(name) + ".freesaber")) {
        PaperLogger.error("FreeSabers: failed to save %s", name.c_str());
        return false;
    }

    LoadPresets();
    EnsureActivePresets();
    lastImportStatus = "Saved " + name + ".freesaber";
    PaperLogger.info("FreeSabers: saved preset %s", name.c_str());
    return true;
}

void Config::LoadImportLog() {
    importLog.clear();
    std::ifstream file(importLogPath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        const auto separator = line.find('\t');
        if (separator == std::string::npos) continue;
        const std::string source = Trim(line.substr(0, separator));
        const std::string name = Trim(line.substr(separator + 1));
        if (!source.empty() && !name.empty()) importLog.emplace_back(source, name);
    }
}

void Config::SaveImportLog() {
    std::ofstream file(importLogPath);
    if (!file.is_open()) return;
    for (const auto& [source, name] : importLog) {
        file << source << '\t' << name << '\n';
    }
}

void Config::LoadSettings() {
    std::string content;
    if (!ReadFile(settingsPath, content)) return;

    rapidjson::Document document;
    document.Parse(content.c_str(), content.size());
    if (document.HasParseError() || !document.IsObject()) {
        PaperLogger.warn("FreeSabers: settings.json is invalid, using defaults");
        return;
    }

    enabled = Json::Bool(document, "enabled", enabled);
    trailsEnabled = Json::Bool(document, "trails", trailsEnabled);
    sparksEnabled = Json::Bool(document, "sparks", sparksEnabled);
    glowEnabled = Json::Bool(document, "glow", glowEnabled);
    activeLeftPreset = Json::String(document, "left", activeLeftPreset);
    activeRightPreset = Json::String(document, "right", activeRightPreset);
}

void Config::SaveSettings() {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("enabled");
    writer.Bool(enabled);
    writer.Key("trails");
    writer.Bool(trailsEnabled);
    writer.Key("sparks");
    writer.Bool(sparksEnabled);
    writer.Key("glow");
    writer.Bool(glowEnabled);
    writer.Key("left");
    writer.String(activeLeftPreset.c_str());
    writer.Key("right");
    writer.String(activeRightPreset.c_str());
    writer.EndObject();

    std::ofstream file(settingsPath);
    if (!file.is_open()) return;
    file << buffer.GetString() << "\n";
}

void Config::SetEnabled(bool value) {
    enabled = value;
    SaveSettings();
}

void Config::SetTrailsEnabled(bool value) {
    trailsEnabled = value;
    SaveSettings();
}

void Config::SetSparksEnabled(bool value) {
    sparksEnabled = value;
    SaveSettings();
}

void Config::SetGlowEnabled(bool value) {
    glowEnabled = value;
    SaveSettings();
}

} // namespace FreeSabers
