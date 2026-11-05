#include "config/SaberConfig.hpp"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>

namespace FreeSabers {

// ============================================================================
// Vector3
// ============================================================================

rapidjson::Value Vector3::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value arr(rapidjson::kArrayType);
    arr.PushBack(x, allocator);
    arr.PushBack(y, allocator);
    arr.PushBack(z, allocator);
    return arr;
}

void Vector3::FromJson(const rapidjson::Value& value) {
    if (value.IsArray() && value.Size() >= 3) {
        x = value[0].GetFloat();
        y = value[1].GetFloat();
        z = value[2].GetFloat();
    }
}

// ============================================================================
// Color
// ============================================================================

rapidjson::Value Color::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value arr(rapidjson::kArrayType);
    arr.PushBack(r, allocator);
    arr.PushBack(g, allocator);
    arr.PushBack(b, allocator);
    arr.PushBack(a, allocator);
    return arr;
}

void Color::FromJson(const rapidjson::Value& value) {
    if (value.IsArray() && value.Size() >= 3) {
        r = value[0].GetFloat();
        g = value[1].GetFloat();
        b = value[2].GetFloat();
        if (value.Size() >= 4) a = value[3].GetFloat();
    }
}

// ============================================================================
// ColorGradient
// ============================================================================

rapidjson::Value ColorGradient::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto& [time, color] : keys) {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.AddMember("time", time, allocator);
        obj.AddMember("color", color.ToJson(allocator), allocator);
        arr.PushBack(obj, allocator);
    }
    return arr;
}

void ColorGradient::FromJson(const rapidjson::Value& value) {
    keys.clear();
    if (value.IsArray()) {
        for (const auto& item : value.GetArray()) {
            float time = item["time"].GetFloat();
            Color color;
            color.FromJson(item["color"]);
            keys.emplace_back(time, color);
        }
        // Sort by time
        std::sort(keys.begin(), keys.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
    }
}

Color ColorGradient::Evaluate(float t) const {
    if (keys.empty()) return Color(1, 1, 1, 1);
    if (keys.size() == 1) return keys[0].second;

    t = std::clamp(t, 0.0f, 1.0f);

    // Find the two keys to interpolate between
    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (t >= keys[i].first && t <= keys[i + 1].first) {
            float localT = (t - keys[i].first) / (keys[i + 1].first - keys[i].first);
            const Color& c1 = keys[i].second;
            const Color& c2 = keys[i + 1].second;
            return Color(
                c1.r + (c2.r - c1.r) * localT,
                c1.g + (c2.g - c1.g) * localT,
                c1.b + (c2.b - c1.b) * localT,
                c1.a + (c2.a - c1.a) * localT
            );
        }
    }

    // If t is beyond last key, return last color
    if (t >= keys.back().first) return keys.back().second;
    return keys.front().second;
}

// ============================================================================
// TrailConfig
// ============================================================================

rapidjson::Value TrailConfig::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("enabled", enabled, allocator);
    obj.AddMember("width", width, allocator);
    obj.AddMember("length", length, allocator);
    obj.AddMember("lifetime", lifetime, allocator);
    obj.AddMember("colorGradient", colorGradient.ToJson(allocator), allocator);
    obj.AddMember("texturePath", rapidjson::Value(texturePath.c_str(), allocator), allocator);
    obj.AddMember("useTexture", useTexture, allocator);
    obj.AddMember("textureMode", textureMode, allocator);
    return obj;
}

void TrailConfig::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("enabled")) enabled = value["enabled"].GetBool();
    if (value.HasMember("width")) width = value["width"].GetFloat();
    if (value.HasMember("length")) length = value["length"].GetFloat();
    if (value.HasMember("lifetime")) lifetime = value["lifetime"].GetFloat();
    if (value.HasMember("colorGradient")) colorGradient.FromJson(value["colorGradient"]);
    if (value.HasMember("texturePath")) texturePath = value["texturePath"].GetString();
    if (value.HasMember("useTexture")) useTexture = value["useTexture"].GetBool();
    if (value.HasMember("textureMode")) textureMode = value["textureMode"].GetInt();
}

// ============================================================================
// GlowConfig
// ============================================================================

rapidjson::Value GlowConfig::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("enabled", enabled, allocator);
    obj.AddMember("intensity", intensity, allocator);
    obj.AddMember("radius", radius, allocator);
    obj.AddMember("color", color.ToJson(allocator), allocator);
    obj.AddMember("pulseGradient", pulseGradient.ToJson(allocator), allocator);
    obj.AddMember("pulseEnabled", pulseEnabled, allocator);
    obj.AddMember("pulseSpeed", pulseSpeed, allocator);
    obj.AddMember("bloomTexturePath", rapidjson::Value(bloomTexturePath.c_str(), allocator), allocator);
    return obj;
}

void GlowConfig::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("enabled")) enabled = value["enabled"].GetBool();
    if (value.HasMember("intensity")) intensity = value["intensity"].GetFloat();
    if (value.HasMember("radius")) radius = value["radius"].GetFloat();
    if (value.HasMember("color")) color.FromJson(value["color"]);
    if (value.HasMember("pulseGradient")) pulseGradient.FromJson(value["pulseGradient"]);
    if (value.HasMember("pulseEnabled")) pulseEnabled = value["pulseEnabled"].GetBool();
    if (value.HasMember("pulseSpeed")) pulseSpeed = value["pulseSpeed"].GetFloat();
    if (value.HasMember("bloomTexturePath")) bloomTexturePath = value["bloomTexturePath"].GetString();
}

// ============================================================================
// SparkConfig
// ============================================================================

rapidjson::Value SparkConfig::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("enabled", enabled, allocator);
    obj.AddMember("count", count, allocator);
    obj.AddMember("size", size, allocator);
    obj.AddMember("lifetime", lifetime, allocator);
    obj.AddMember("velocity", velocity, allocator);
    obj.AddMember("spread", spread, allocator);
    obj.AddMember("colorGradient", colorGradient.ToJson(allocator), allocator);
    obj.AddMember("texturePath", rapidjson::Value(texturePath.c_str(), allocator), allocator);
    obj.AddMember("useCustomTexture", useCustomTexture, allocator);
    obj.AddMember("emissionMode", emissionMode, allocator);
    return obj;
}

void SparkConfig::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("enabled")) enabled = value["enabled"].GetBool();
    if (value.HasMember("count")) count = value["count"].GetInt();
    if (value.HasMember("size")) size = value["size"].GetFloat();
    if (value.HasMember("lifetime")) lifetime = value["lifetime"].GetFloat();
    if (value.HasMember("velocity")) velocity = value["velocity"].GetFloat();
    if (value.HasMember("spread")) spread = value["spread"].GetFloat();
    if (value.HasMember("colorGradient")) colorGradient.FromJson(value["colorGradient"]);
    if (value.HasMember("texturePath")) texturePath = value["texturePath"].GetString();
    if (value.HasMember("useCustomTexture")) useCustomTexture = value["useCustomTexture"].GetBool();
    if (value.HasMember("emissionMode")) emissionMode = value["emissionMode"].GetInt();
}

// ============================================================================
// DynamicJointConfig
// ============================================================================

rapidjson::Value DynamicJointConfig::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("enabled", enabled, allocator);
    obj.AddMember("jointCount", jointCount, allocator);
    obj.AddMember("stiffness", stiffness, allocator);
    obj.AddMember("damping", damping, allocator);
    obj.AddMember("mass", mass, allocator);
    obj.AddMember("gravity", gravity, allocator);
    obj.AddMember("maxAngle", maxAngle, allocator);
    obj.AddMember("localAxis", localAxis.ToJson(allocator), allocator);
    obj.AddMember("affectBlade", affectBlade, allocator);
    obj.AddMember("affectHilt", affectHilt, allocator);
    return obj;
}

void DynamicJointConfig::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("enabled")) enabled = value["enabled"].GetBool();
    if (value.HasMember("jointCount")) jointCount = value["jointCount"].GetInt();
    if (value.HasMember("stiffness")) stiffness = value["stiffness"].GetFloat();
    if (value.HasMember("damping")) damping = value["damping"].GetFloat();
    if (value.HasMember("mass")) mass = value["mass"].GetFloat();
    if (value.HasMember("gravity")) gravity = value["gravity"].GetFloat();
    if (value.HasMember("maxAngle")) maxAngle = value["maxAngle"].GetFloat();
    if (value.HasMember("localAxis")) localAxis.FromJson(value["localAxis"]);
    if (value.HasMember("affectBlade")) affectBlade = value["affectBlade"].GetBool();
    if (value.HasMember("affectHilt")) affectHilt = value["affectHilt"].GetBool();
}

// ============================================================================
// MaterialDriver
// ============================================================================

rapidjson::Value MaterialDriver::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
    obj.AddMember("type", static_cast<int>(type), allocator);
    obj.AddMember("targetProperty", rapidjson::Value(targetProperty.c_str(), allocator), allocator);
    obj.AddMember("minValue", minValue, allocator);
    obj.AddMember("maxValue", maxValue, allocator);
    obj.AddMember("multiplier", multiplier, allocator);
    obj.AddMember("offset", offset, allocator);
    obj.AddMember("useGradient", useGradient, allocator);
    if (useGradient) obj.AddMember("gradient", gradient.ToJson(allocator), allocator);
    obj.AddMember("materialIndex", materialIndex, allocator);
    obj.AddMember("shaderKeyword", rapidjson::Value(shaderKeyword.c_str(), allocator), allocator);
    return obj;
}

void MaterialDriver::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("name")) name = value["name"].GetString();
    if (value.HasMember("type")) type = static_cast<DriverType>(value["type"].GetInt());
    if (value.HasMember("targetProperty")) targetProperty = value["targetProperty"].GetString();
    if (value.HasMember("minValue")) minValue = value["minValue"].GetFloat();
    if (value.HasMember("maxValue")) maxValue = value["maxValue"].GetFloat();
    if (value.HasMember("multiplier")) multiplier = value["multiplier"].GetFloat();
    if (value.HasMember("offset")) offset = value["offset"].GetFloat();
    if (value.HasMember("useGradient")) useGradient = value["useGradient"].GetBool();
    if (value.HasMember("gradient")) gradient.FromJson(value["gradient"]);
    if (value.HasMember("materialIndex")) materialIndex = value["materialIndex"].GetInt();
    if (value.HasMember("shaderKeyword")) shaderKeyword = value["shaderKeyword"].GetString();
}

float MaterialDriver::Evaluate(float inputValue) const {
    float normalized = std::clamp((inputValue - minValue) / (maxValue - minValue), 0.0f, 1.0f);
    float result = normalized * multiplier + offset;

    if (useGradient && !gradient.keys.empty()) {
        Color c = gradient.Evaluate(normalized);
        // Return average of RGB as float value
        result = (c.r + c.g + c.b) / 3.0f;
    }

    return result;
}

// ============================================================================
// LayerConfig
// ============================================================================

rapidjson::Value LayerConfig::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
    obj.AddMember("type", static_cast<int>(type), allocator);
    obj.AddMember("enabled", enabled, allocator);
    obj.AddMember("order", order, allocator);
    obj.AddMember("modelPath", rapidjson::Value(modelPath.c_str(), allocator), allocator);
    obj.AddMember("scale", scale.ToJson(allocator), allocator);
    obj.AddMember("positionOffset", positionOffset.ToJson(allocator), allocator);
    obj.AddMember("rotationOffset", rotationOffset.ToJson(allocator), allocator);
    obj.AddMember("materialPath", rapidjson::Value(materialPath.c_str(), allocator), allocator);
    obj.AddMember("baseColor", baseColor.ToJson(allocator), allocator);
    obj.AddMember("emissionColor", emissionColor.ToJson(allocator), allocator);
    obj.AddMember("emissionIntensity", emissionIntensity, allocator);
    obj.AddMember("smoothness", smoothness, allocator);
    obj.AddMember("metallic", metallic, allocator);
    obj.AddMember("trail", trail.ToJson(allocator), allocator);
    obj.AddMember("glow", glow.ToJson(allocator), allocator);
    obj.AddMember("sparks", sparks.ToJson(allocator), allocator);
    obj.AddMember("dynamicJoints", dynamicJoints.ToJson(allocator), allocator);

    rapidjson::Value driversArr(rapidjson::kArrayType);
    for (const auto& driver : drivers) {
        driversArr.PushBack(driver.ToJson(allocator), allocator);
    }
    obj.AddMember("drivers", driversArr, allocator);

    return obj;
}

void LayerConfig::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("name")) name = value["name"].GetString();
    if (value.HasMember("type")) type = static_cast<LayerType>(value["type"].GetInt());
    if (value.HasMember("enabled")) enabled = value["enabled"].GetBool();
    if (value.HasMember("order")) order = value["order"].GetInt();
    if (value.HasMember("modelPath")) modelPath = value["modelPath"].GetString();
    if (value.HasMember("scale")) scale.FromJson(value["scale"]);
    if (value.HasMember("positionOffset")) positionOffset.FromJson(value["positionOffset"]);
    if (value.HasMember("rotationOffset")) rotationOffset.FromJson(value["rotationOffset"]);
    if (value.HasMember("materialPath")) materialPath = value["materialPath"].GetString();
    if (value.HasMember("baseColor")) baseColor.FromJson(value["baseColor"]);
    if (value.HasMember("emissionColor")) emissionColor.FromJson(value["emissionColor"]);
    if (value.HasMember("emissionIntensity")) emissionIntensity = value["emissionIntensity"].GetFloat();
    if (value.HasMember("smoothness")) smoothness = value["smoothness"].GetFloat();
    if (value.HasMember("metallic")) metallic = value["metallic"].GetFloat();
    if (value.HasMember("trail")) trail.FromJson(value["trail"]);
    if (value.HasMember("glow")) glow.FromJson(value["glow"]);
    if (value.HasMember("sparks")) sparks.FromJson(value["sparks"]);
    if (value.HasMember("dynamicJoints")) dynamicJoints.FromJson(value["dynamicJoints"]);

    drivers.clear();
    if (value.HasMember("drivers") && value["drivers"].IsArray()) {
        for (const auto& item : value["drivers"].GetArray()) {
            MaterialDriver driver;
            driver.FromJson(item);
            drivers.push_back(driver);
        }
    }
}

// ============================================================================
// SaberPreset
// ============================================================================

rapidjson::Value SaberPreset::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("id", rapidjson::Value(id.c_str(), allocator), allocator);
    obj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
    obj.AddMember("author", rapidjson::Value(author.c_str(), allocator), allocator);
    obj.AddMember("description", rapidjson::Value(description.c_str(), allocator), allocator);
    obj.AddMember("version", version, allocator);

    rapidjson::Value layersArr(rapidjson::kArrayType);
    for (const auto& layer : layers) {
        layersArr.PushBack(layer.ToJson(allocator), allocator);
    }
    obj.AddMember("layers", layersArr, allocator);

    rapidjson::Value driversArr(rapidjson::kArrayType);
    for (const auto& driver : globalDrivers) {
        driversArr.PushBack(driver.ToJson(allocator), allocator);
    }
    obj.AddMember("globalDrivers", driversArr, allocator);

    return obj;
}

void SaberPreset::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("id")) id = value["id"].GetString();
    if (value.HasMember("name")) name = value["name"].GetString();
    if (value.HasMember("author")) author = value["author"].GetString();
    if (value.HasMember("description")) description = value["description"].GetString();
    if (value.HasMember("version")) version = value["version"].GetInt();

    layers.clear();
    if (value.HasMember("layers") && value["layers"].IsArray()) {
        for (const auto& item : value["layers"].GetArray()) {
            LayerConfig layer;
            layer.FromJson(item);
            layers.push_back(layer);
        }
        // Sort by order
        std::sort(layers.begin(), layers.end(), [](const LayerConfig& a, const LayerConfig& b) {
            return a.order < b.order;
        });
    }

    globalDrivers.clear();
    if (value.HasMember("globalDrivers") && value["globalDrivers"].IsArray()) {
        for (const auto& item : value["globalDrivers"].GetArray()) {
            MaterialDriver driver;
            driver.FromJson(item);
            globalDrivers.push_back(driver);
        }
    }
}

bool SaberPreset::Validate(std::string& errorOut) const {
    if (name.empty()) {
        errorOut = "Preset name cannot be empty";
        return false;
    }
    if (layers.empty()) {
        errorOut = "Preset must have at least one layer";
        return false;
    }
    // Check for at least one blade layer
    bool hasBlade = false;
    for (const auto& layer : layers) {
        if (layer.type == LayerType::Blade && layer.enabled) {
            hasBlade = true;
            break;
        }
    }
    if (!hasBlade) {
        errorOut = "Preset must have at least one enabled Blade layer";
        return false;
    }
    return true;
}

// ============================================================================
// FreeSaberConfig
// ============================================================================

rapidjson::Value FreeSaberConfig::ToJson(rapidjson::Document::AllocatorType& allocator) const {
    rapidjson::Value obj(rapidjson::kObjectType);
    obj.AddMember("formatVersion", formatVersion, allocator);
    obj.AddMember("name", rapidjson::Value(name.c_str(), allocator), allocator);
    obj.AddMember("author", rapidjson::Value(author.c_str(), allocator), allocator);
    obj.AddMember("description", rapidjson::Value(description.c_str(), allocator), allocator);
    obj.AddMember("createdDate", rapidjson::Value(createdDate.c_str(), allocator), allocator);
    obj.AddMember("modifiedDate", rapidjson::Value(modifiedDate.c_str(), allocator), allocator);
    obj.AddMember("useSameForBoth", useSameForBoth, allocator);
    obj.AddMember("leftSaber", leftSaber.ToJson(allocator), allocator);
    obj.AddMember("rightSaber", rightSaber.ToJson(allocator), allocator);
    return obj;
}

void FreeSaberConfig::FromJson(const rapidjson::Value& value) {
    if (value.HasMember("formatVersion")) formatVersion = value["formatVersion"].GetInt();
    if (value.HasMember("name")) name = value["name"].GetString();
    if (value.HasMember("author")) author = value["author"].GetString();
    if (value.HasMember("description")) description = value["description"].GetString();
    if (value.HasMember("createdDate")) createdDate = value["createdDate"].GetString();
    if (value.HasMember("modifiedDate")) modifiedDate = value["modifiedDate"].GetString();
    if (value.HasMember("useSameForBoth")) useSameForBoth = value["useSameForBoth"].GetBool();
    if (value.HasMember("leftSaber")) leftSaber.FromJson(value["leftSaber"]);
    if (value.HasMember("rightSaber")) rightSaber.FromJson(value["rightSaber"]);
}

std::string FreeSaberConfig::ToJsonString(bool pretty) const {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("FreeSaber", ToJson(allocator), allocator);

    rapidjson::StringBuffer buffer;
    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
    } else {
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
    }
    return buffer.GetString();
}

bool FreeSaberConfig::FromJsonString(const std::string& jsonStr) {
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());
    if (doc.HasParseError()) return false;
    if (doc.HasMember("FreeSaber")) {
        FromJson(doc["FreeSaber"]);
        return true;
    }
    return false;
}

bool FreeSaberConfig::SaveToFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    // Update modified date
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

    FreeSaberConfig config = *this;
    config.modifiedDate = ss.str();

    file << config.ToJsonString(true);
    return true;
}

std::optional<FreeSaberConfig> FreeSaberConfig::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return std::nullopt;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    FreeSaberConfig config;
    if (config.FromJsonString(content)) {
        return config;
    }
    return std::nullopt;
}

bool FreeSaberConfig::Validate(std::string& errorOut) const {
    std::string leftError, rightError;
    if (!leftSaber.Validate(leftError)) {
        errorOut = "Left saber: " + leftError;
        return false;
    }
    if (!useSameForBoth && !rightSaber.Validate(rightError)) {
        errorOut = "Right saber: " + rightError;
        return false;
    }
    return true;
}

} // namespace FreeSabers