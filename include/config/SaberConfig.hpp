#pragma once

#include <optional>
#include <string>
#include <vector>

namespace FreeSabers {

// ============================================================================
// Basic value types
// ============================================================================

struct Vec2 {
    float x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static Color Lerp(const Color& from, const Color& to, float t);
    static Color FromHSV(float h, float s, float v, float a = 1.0f);
    Color WithAlpha(float newAlpha) const { return Color(r, g, b, newAlpha); }
    Color MultiplyRGB(float scale) const { return Color(r * scale, g * scale, b * scale, a); }

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

// ============================================================================
// Curves and gradients
// ============================================================================

// Interpolation values match the ReeSabers editor:
// 0 = Ease, 1 = Linear, 2 = Constant, 3 = Smooth.
struct CurvePoint {
    float time = 0.0f;
    float value = 0.0f;
};

struct Curve {
    int interpolationType = 1;
    std::vector<CurvePoint> points;
    float valueFrom = 0.0f;
    float valueTo = 1.0f;

    bool Empty() const { return points.empty(); }
    float Evaluate(float t) const;
    // Evaluates an external driver value mapped through valueFrom..valueTo.
    float EvaluateDriver(float raw) const;
};

struct GradientPoint {
    float time = 0.0f;
    Color value = Color();
};

struct ColorCurve {
    int interpolationType = 1;
    std::vector<GradientPoint> points;
    float valueFrom = 0.0f;
    float valueTo = 1.0f;

    bool Empty() const { return points.empty(); }
    Color Evaluate(float t) const;
    Color EvaluateDriver(float raw) const;
};

struct TransformOffset {
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 rotation = Vector3(0.0f, 0.0f, 0.0f); // euler degrees
    Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);

    bool IsIdentity() const;
};

// ============================================================================
// Colouring
// ============================================================================

// ReeSabers colour override / hue driver settings.
struct ColorOverride {
    bool force = false;
    int type = 0;
    float hue = 0.0f;               // 0..1
    float saturation = 1.0f;
    float value = 1.0f;
    float hueShiftPerSecond = 0.0f;
    float fakeGlowMultiplier = 1.0f;
    int colorSource = 0;

    // time in seconds since the saber was created.
    Color Evaluate(float time) const;
};

// ============================================================================
// Drivers
// ============================================================================

// Driver types follow the ReeSabers valueType enum.
enum class DriverType : int {
    None = 0,
    TipVelocity = 1,
    SwingsPerSecond = 2,
    TimeDependence = 3,
    TipAcceleration = 4,
    AngularVelocity = 5,
    ScorePercentage = 6,
    Combo = 7,
    Misses = 8,
    Energy = 9,
    Multiplier = 10,
    CutScore = 11,
    Accuracy = 12,
    PreCutScore = 13,
    PostCutScore = 14,
    CutsPerSecond = 15,
    TimeAfterCut = 16,
    DirectionX = 17,
    DirectionY = 18,
    DirectionZ = 19
};

struct MaterialDriver {
    std::string name;
    int valueType = 0;
    float increaseResistance = 0.0f;
    float decreaseResistance = 0.0f;
    Curve valueOverValue;
    ColorCurve colorOverValue;
    float valueFrom = 0.0f;
    float valueTo = 1.0f;

    float EvaluateValue(float raw) const;
    Color EvaluateColor(float raw) const;
};

// ============================================================================
// Blade
// ============================================================================

struct BladeConfig {
    bool enabled = true;

    // Geometry (saber local space, blade runs along +Z).
    float zOffsetFrom = -0.17f;
    float zOffsetTo = 1.0f;
    float radius = 0.0132f;
    float stretchBias = 4.0f;
    Curve profile;                  // relative radius along the blade (0..1)
    bool startCap = true;
    bool endCap = true;
    int verticalResolution = 20;
    int horizontalResolution = 10;

    // Colours / material.
    Color baseColor = Color(0.0f, 0.8f, 1.0f, 1.0f);
    Color emissionColor = Color(0.0f, 0.8f, 1.0f, 1.0f);
    float emissionIntensity = 2.0f;
    Color reflectionColor = Color(0.05f, 0.005f, 0.005f, 0.0f);
    Color envLightColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
    float fresnelPower = 5.0f;
    float metallic = 0.0f;
    float roughness = 0.43f;
    float envLightIntensity = 1.0f;
    float reflectionIntensity = 1.0f;
    float normalMapIntensity = 1.0f;
    bool sceneReflections = false;
    bool sceneLights = false;
    float glowMultiplier = 1.0f;
    Color handleColor = Color(0.1f, 0.1f, 0.1f, 0.0f);
    float handleRoughness = 2.0f;
    int renderQueue = 3002;
    int cullMode = 0;               // 0 = off, 1 = front, 2 = back
    bool depthWrite = false;

    // Motion blur (blur-frames are converted to a trail duration in the renderer).
    float blurFrames = 2.0f;
    float blurSensitivity = 0.6f;

    // ReeSabers "mask" mappings sampled along the blade.
    ColorCurve colorOverValue;
    Curve alphaOverValue;
    Curve scaleOverValue;
    float valueFrom = 0.0f;
    float valueTo = 1.0f;
    Curve handleMask;
    int bladeMaskResolution = 48;

    // Drivers (per-blade, evaluated per-frame and applied to material).
    std::vector<MaterialDriver> drivers;

    // Textures (tiling / scrolling only; custom textures are not imported).
    Vec2 tiling = Vec2(1.0f, 1.0f);
    Vec2 uvScroll = Vec2(0.0f, 0.0f);
    bool useTexture = false;
    std::string texturePath;

    ColorOverride colorOverride;

    Color SampleColor(float t) const;
    float SampleAlpha(float t) const;
    float SampleScale(float t) const;
};

// ============================================================================
// Trail
// ============================================================================

struct TrailConfig {
    bool enabled = true;
    std::string name = "Trail";

    // 0 = tip trail (billboard ribbon), 1 = blade trail (ribbon between both
    // blade edges, like the vanilla saber trail).
    int trailType = 1;
    float length = 0.22f;           // seconds of motion kept in the trail
    float width = 0.02f;            // metres (tip trails)
    float offset = 1.0f;            // position along the blade (1 = tip)
    float distortionMultiplier = 1.0f;
    int horizontalResolution = 4;
    int verticalResolution = 80;
    int blendingMode = 0;
    bool alwaysOnTop = false;
    int renderQueue = 3000;
    float minVertexDistance = 0.004f;

    ColorCurve colorOverAge;
    Curve alphaOverAge;
    Curve widthOverAge;

    // Extra motion-blur strength driven by blade speed.
    float speedWidth = 0.0f;
    float speedAlpha = 0.25f;
    float blurFrames = 0.0f;

    bool useTexture = false;
    std::string texturePath;
    Vec2 tiling = Vec2(1.0f, 1.0f);
    Vec2 uvScroll = Vec2(0.0f, 0.0f);

    TransformOffset transform;
    std::vector<MaterialDriver> drivers;

    bool IsTipTrail() const { return trailType == 0; }
};

// ============================================================================
// Glow
// ============================================================================

struct GlowConfig {
    bool enabled = true;
    float intensity = 1.5f;
    float radius = 0.4f;            // extra shell radius multiplier
    Color color = Color(0.0f, 0.8f, 1.0f, 1.0f);
    ColorCurve pulseGradient;
    bool pulseEnabled = false;
    float pulseSpeed = 1.0f;
    float pulseAmount = 0.35f;
    bool useLight = false;          // optional real Unity light (Quest: keep off)
    float lightRange = 2.0f;
};

// ============================================================================
// Sparks (cut particles)
// ============================================================================

struct SparkConfig {
    bool enabled = true;
    int count = 96;                 // live particle budget (hard capped at runtime)
    float size = 0.03f;
    float minLifetime = 0.1f;
    float maxLifetime = 0.5f;
    float velocity = 1.5f;
    float spread = 35.0f;           // degrees
    float trailLength = 0.15f;
    float trailWidth = 0.05f;
    float gravityY = 0.0f;
    float damping = 1.5f;
    float turbulencePower = 0.0f;
    float turbulenceScale = 4.0f;
    int emissionMode = 1;           // 0 = cut, 1 = cut burst, 2 = continuous while swinging
    float triggerPercentage = 0.7f;
    float triggerPeriod = 0.05f;
    float burstSpread = 0.8f;

    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    ColorCurve colorOverLifetime;
    Curve alphaOverLifetime;
    Curve scaleOverLifetime;

    TransformOffset transform;

    // Drivers (per-spark-system, evaluated per-frame and modulating particle properties).
    std::vector<MaterialDriver> drivers;

    bool useTexture = false;
    std::string texturePath;
};

// ============================================================================
// Dynamic joints ("bolts" hanging from the blade)
// ============================================================================

struct BoltsConfig {
    bool enabled = false;
    int joints = 4;
    float spacing = 0.06f;          // metres between joints
    float size = 0.02f;             // joint radius
    float stiffness = 60.0f;
    float damping = 8.0f;
    float gravityY = -1.5f;
    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
};

// ============================================================================
// Preset / config
// ============================================================================

struct SaberPreset {
    std::string id;
    std::string name = "Saber";
    std::string author = "Unknown";
    std::string description;
    int version = 2;

    TransformOffset transform;

    BladeConfig blade;
    std::vector<TrailConfig> trails;
    SparkConfig sparks;
    GlowConfig glow;
    BoltsConfig bolts;

    // true = keep the game's saber model and only add effects.
    bool keepVanillaModel = false;

    std::vector<MaterialDriver> drivers;
};

struct FreeSaberConfig {
    int formatVersion = 2;
    std::string name = "Untitled Saber";
    std::string author = "Unknown";
    std::string description;
    std::string sourceFormat = "FreeSabers";
    std::string sourceVersion;
    std::string createdDate;
    std::string modifiedDate;

    bool useSameForBoth = true;
    SaberPreset leftSaber;
    SaberPreset rightSaber;

    FreeSaberConfig() = default;

    std::string ToJsonString(bool pretty = true) const;
    bool FromJsonString(const std::string& jsonStr);

    bool SaveToFile(const std::string& filePath) const;
    static std::optional<FreeSaberConfig> LoadFromFile(const std::string& filePath);

    bool Validate(std::string& errorOut) const;

    // Resolves the preset for a side (mirrors the left preset when
    // useSameForBoth is true).
    const SaberPreset& GetPreset(bool isLeft) const;
};

} // namespace FreeSabers
