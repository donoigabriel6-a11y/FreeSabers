#pragma once

#include <string>
#include <vector>
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace FreeSabers {

// Forward declarations
struct ColorGradient;
struct TrailConfig;
struct GlowConfig;
struct SparkConfig;
struct DynamicJointConfig;
struct MaterialDriver;
struct LayerConfig;
struct SaberPreset;
struct FreeSaberConfig;

// ============================================================================
// Basic Types
// ============================================================================

struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

struct ColorGradient {
    std::vector<std::pair<float, Color>> keys; // time (0-1), color

    ColorGradient() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);

    Color Evaluate(float t) const;
};

// ============================================================================
// Effect Configurations
// ============================================================================

struct TrailConfig {
    bool enabled = true;
    float width = 0.1f;
    float length = 2.0f;
    float lifetime = 0.5f;
    ColorGradient colorGradient;
    std::string texturePath;
    bool useTexture = false;
    int textureMode = 0; // 0 = stretch, 1 = tile, 2 = static

    TrailConfig() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

struct GlowConfig {
    bool enabled = true;
    float intensity = 1.0f;
    float radius = 0.5f;
    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    ColorGradient pulseGradient;
    bool pulseEnabled = false;
    float pulseSpeed = 1.0f;
    std::string bloomTexturePath;

    GlowConfig() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

struct SparkConfig {
    bool enabled = true;
    int count = 10;
    float size = 0.05f;
    float lifetime = 0.3f;
    float velocity = 2.0f;
    float spread = 45.0f; // degrees
    ColorGradient colorGradient;
    std::string texturePath;
    bool useCustomTexture = false;
    int emissionMode = 0; // 0 = on cut, 1 = continuous, 2 = on swing

    SparkConfig() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

struct DynamicJointConfig {
    bool enabled = true;
    int jointCount = 5;
    float stiffness = 0.5f;
    float damping = 0.1f;
    float mass = 0.1f;
    float gravity = -9.81f;
    float maxAngle = 45.0f; // degrees
    Vector3 localAxis = Vector3(0, 1, 0);
    bool affectBlade = true;
    bool affectHilt = false;

    DynamicJointConfig() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

// ============================================================================
// Material Driver System
// ============================================================================

enum class DriverType {
    Score,
    Combo,
    SwingSpeed,
    AudioBass,
    AudioMid,
    AudioTreble,
    Time,
    Energy,
    Accuracy,
    Custom
};

struct MaterialDriver {
    std::string name;
    DriverType type = DriverType::Score;
    std::string targetProperty; // e.g., "_EmissionColor", "_GlowIntensity", "_TrailWidth"
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float multiplier = 1.0f;
    float offset = 0.0f;
    bool useGradient = false;
    ColorGradient gradient;
    int materialIndex = 0; // which material on the saber
    std::string shaderKeyword; // for enabling/disabling features

    MaterialDriver() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);

    float Evaluate(float inputValue) const;
};

// ============================================================================
// Layer Configuration (for stacking multiple effects)
// ============================================================================

enum class LayerType {
    Blade,
    Trail,
    Glow,
    Sparks,
    DynamicJoints,
    Hilt,
    Custom
};

struct LayerConfig {
    std::string name;
    LayerType type = LayerType::Blade;
    bool enabled = true;
    int order = 0; // rendering order

    // Model/Mesh
    std::string modelPath; // .obj, .whacker, or built-in
    Vector3 scale = Vector3(1, 1, 1);
    Vector3 positionOffset = Vector3(0, 0, 0);
    Vector3 rotationOffset = Vector3(0, 0, 0);

    // Material
    std::string materialPath; // .mat or built-in
    Color baseColor = Color(1, 1, 1, 1);
    Color emissionColor = Color(0, 0, 0, 0);
    float emissionIntensity = 0.0f;
    float smoothness = 0.5f;
    float metallic = 0.0f;

    // Effects (only used based on layer type)
    TrailConfig trail;
    GlowConfig glow;
    SparkConfig sparks;
    DynamicJointConfig dynamicJoints;

    // Drivers
    std::vector<MaterialDriver> drivers;

    LayerConfig() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);
};

// ============================================================================
// Saber Preset (Left/Right configuration)
// ============================================================================

struct SaberPreset {
    std::string id;
    std::string name;
    std::string author;
    std::string description;
    int version = 1;

    std::vector<LayerConfig> layers;
    std::vector<MaterialDriver> globalDrivers;

    SaberPreset() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);

    bool Validate(std::string& errorOut) const;
};

// ============================================================================
// Main Config (FreeSaber file format)
// ============================================================================

struct FreeSaberConfig {
    int formatVersion = 1;
    std::string name = "Untitled Saber";
    std::string author = "Unknown";
    std::string description = "";
    std::string createdDate;
    std::string modifiedDate;

    SaberPreset leftSaber;
    SaberPreset rightSaber;

    // Global settings
    bool useSameForBoth = true; // if true, rightSaber mirrors leftSaber

    FreeSaberConfig() = default;

    rapidjson::Value ToJson(rapidjson::Document::AllocatorType& allocator) const;
    void FromJson(const rapidjson::Value& value);

    std::string ToJsonString(bool pretty = true) const;
    bool FromJsonString(const std::string& jsonStr);

    bool SaveToFile(const std::string& filePath) const;
    static std::optional<FreeSaberConfig> LoadFromFile(const std::string& filePath);

    bool Validate(std::string& errorOut) const;
};

} // namespace FreeSabers