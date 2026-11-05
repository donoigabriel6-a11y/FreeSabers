#include "config/SaberConfig.hpp"
#include "config/JsonHelpers.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>

namespace FreeSabers {

namespace {

using Allocator = rapidjson::Document::AllocatorType;

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float ApplyInterpolation(int interpolationType, float t) {
    switch (interpolationType) {
        case 0: // ease
        case 3: // smooth
            return t * t * (3.0f - 2.0f * t);
        case 2: // constant (hold previous key)
            return 0.0f;
        default: // linear
            return t;
    }
}

std::string NowString() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// ---------------------------------------------------------------------------
// JSON writing
// ---------------------------------------------------------------------------

void WriteColor(rapidjson::Value& object, const char* key, const Color& color, Allocator& allocator) {
    rapidjson::Value array(rapidjson::kArrayType);
    array.PushBack(color.r, allocator);
    array.PushBack(color.g, allocator);
    array.PushBack(color.b, allocator);
    array.PushBack(color.a, allocator);
    object.AddMember(rapidjson::Value(key, allocator), array, allocator);
}

void WriteVec2(rapidjson::Value& object, const char* key, const Vec2& value, Allocator& allocator) {
    rapidjson::Value array(rapidjson::kArrayType);
    array.PushBack(value.x, allocator);
    array.PushBack(value.y, allocator);
    object.AddMember(rapidjson::Value(key, allocator), array, allocator);
}

void WriteVector3(rapidjson::Value& object, const char* key, const Vector3& value, Allocator& allocator) {
    rapidjson::Value array(rapidjson::kArrayType);
    array.PushBack(value.x, allocator);
    array.PushBack(value.y, allocator);
    array.PushBack(value.z, allocator);
    object.AddMember(rapidjson::Value(key, allocator), array, allocator);
}

void WriteString(rapidjson::Value& object, const char* key, const std::string& value, Allocator& allocator) {
    object.AddMember(rapidjson::Value(key, allocator), rapidjson::Value(value.c_str(), allocator), allocator);
}

void WriteCurve(rapidjson::Value& object, const char* key, const Curve& curve, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("interpolationType", curve.interpolationType, allocator);
    value.AddMember("valueFrom", curve.valueFrom, allocator);
    value.AddMember("valueTo", curve.valueTo, allocator);

    rapidjson::Value points(rapidjson::kArrayType);
    for (const auto& point : curve.points) {
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("time", point.time, allocator);
        item.AddMember("value", point.value, allocator);
        points.PushBack(item, allocator);
    }
    value.AddMember("points", points, allocator);
    object.AddMember(rapidjson::Value(key, allocator), value, allocator);
}

void WriteColorCurve(rapidjson::Value& object, const char* key, const ColorCurve& curve, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("interpolationType", curve.interpolationType, allocator);
    value.AddMember("valueFrom", curve.valueFrom, allocator);
    value.AddMember("valueTo", curve.valueTo, allocator);

    rapidjson::Value points(rapidjson::kArrayType);
    for (const auto& point : curve.points) {
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("time", point.time, allocator);
        rapidjson::Value color(rapidjson::kArrayType);
        color.PushBack(point.value.r, allocator);
        color.PushBack(point.value.g, allocator);
        color.PushBack(point.value.b, allocator);
        color.PushBack(point.value.a, allocator);
        item.AddMember("value", color, allocator);
        points.PushBack(item, allocator);
    }
    value.AddMember("points", points, allocator);
    object.AddMember(rapidjson::Value(key, allocator), value, allocator);
}

void WriteTransform(rapidjson::Value& object, const TransformOffset& transform, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    WriteVector3(value, "position", transform.position, allocator);
    WriteVector3(value, "rotation", transform.rotation, allocator);
    WriteVector3(value, "scale", transform.scale, allocator);
    object.AddMember("transform", value, allocator);
}

void WriteColorOverride(rapidjson::Value& object, const ColorOverride& override, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("force", override.force, allocator);
    value.AddMember("type", override.type, allocator);
    value.AddMember("hue", override.hue, allocator);
    value.AddMember("saturation", override.saturation, allocator);
    value.AddMember("value", override.value, allocator);
    value.AddMember("hueShiftPerSecond", override.hueShiftPerSecond, allocator);
    value.AddMember("fakeGlowMultiplier", override.fakeGlowMultiplier, allocator);
    value.AddMember("colorSource", override.colorSource, allocator);
    object.AddMember("colorOverride", value, allocator);
}

void WriteDriver(rapidjson::Value& array, const MaterialDriver& driver, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    WriteString(value, "name", driver.name, allocator);
    value.AddMember("valueType", driver.valueType, allocator);
    value.AddMember("increaseResistance", driver.increaseResistance, allocator);
    value.AddMember("decreaseResistance", driver.decreaseResistance, allocator);
    value.AddMember("valueFrom", driver.valueFrom, allocator);
    value.AddMember("valueTo", driver.valueTo, allocator);
    WriteCurve(value, "valueOverValue", driver.valueOverValue, allocator);
    WriteColorCurve(value, "colorOverValue", driver.colorOverValue, allocator);
    array.PushBack(value, allocator);
}

void WriteBlade(rapidjson::Value& presetValue, const BladeConfig& blade, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("enabled", blade.enabled, allocator);
    value.AddMember("zOffsetFrom", blade.zOffsetFrom, allocator);
    value.AddMember("zOffsetTo", blade.zOffsetTo, allocator);
    value.AddMember("radius", blade.radius, allocator);
    value.AddMember("stretchBias", blade.stretchBias, allocator);
    WriteCurve(value, "profile", blade.profile, allocator);
    value.AddMember("startCap", blade.startCap, allocator);
    value.AddMember("endCap", blade.endCap, allocator);
    value.AddMember("verticalResolution", blade.verticalResolution, allocator);
    value.AddMember("horizontalResolution", blade.horizontalResolution, allocator);

    WriteColor(value, "baseColor", blade.baseColor, allocator);
    WriteColor(value, "emissionColor", blade.emissionColor, allocator);
    value.AddMember("emissionIntensity", blade.emissionIntensity, allocator);
    WriteColor(value, "reflectionColor", blade.reflectionColor, allocator);
    WriteColor(value, "envLightColor", blade.envLightColor, allocator);
    value.AddMember("fresnelPower", blade.fresnelPower, allocator);
    value.AddMember("metallic", blade.metallic, allocator);
    value.AddMember("roughness", blade.roughness, allocator);
    value.AddMember("envLightIntensity", blade.envLightIntensity, allocator);
    value.AddMember("reflectionIntensity", blade.reflectionIntensity, allocator);
    value.AddMember("normalMapIntensity", blade.normalMapIntensity, allocator);
    value.AddMember("sceneReflections", blade.sceneReflections, allocator);
    value.AddMember("sceneLights", blade.sceneLights, allocator);
    value.AddMember("glowMultiplier", blade.glowMultiplier, allocator);
    WriteColor(value, "handleColor", blade.handleColor, allocator);
    value.AddMember("handleRoughness", blade.handleRoughness, allocator);
    value.AddMember("renderQueue", blade.renderQueue, allocator);
    value.AddMember("cullMode", blade.cullMode, allocator);
    value.AddMember("depthWrite", blade.depthWrite, allocator);
    value.AddMember("blurFrames", blade.blurFrames, allocator);
    value.AddMember("blurSensitivity", blade.blurSensitivity, allocator);

    WriteColorCurve(value, "colorOverValue", blade.colorOverValue, allocator);
    WriteCurve(value, "alphaOverValue", blade.alphaOverValue, allocator);
    WriteCurve(value, "scaleOverValue", blade.scaleOverValue, allocator);
    value.AddMember("valueFrom", blade.valueFrom, allocator);
    value.AddMember("valueTo", blade.valueTo, allocator);
    WriteCurve(value, "handleMask", blade.handleMask, allocator);
    value.AddMember("bladeMaskResolution", blade.bladeMaskResolution, allocator);

    WriteVec2(value, "tiling", blade.tiling, allocator);
    WriteVec2(value, "uvScroll", blade.uvScroll, allocator);
    value.AddMember("useTexture", blade.useTexture, allocator);
    WriteString(value, "texturePath", blade.texturePath, allocator);

    WriteColorOverride(value, blade.colorOverride, allocator);
    presetValue.AddMember("blade", value, allocator);
}

void WriteTrail(rapidjson::Value& array, const TrailConfig& trail, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("enabled", trail.enabled, allocator);
    WriteString(value, "name", trail.name, allocator);
    value.AddMember("trailType", trail.trailType, allocator);
    value.AddMember("length", trail.length, allocator);
    value.AddMember("width", trail.width, allocator);
    value.AddMember("offset", trail.offset, allocator);
    value.AddMember("distortionMultiplier", trail.distortionMultiplier, allocator);
    value.AddMember("horizontalResolution", trail.horizontalResolution, allocator);
    value.AddMember("verticalResolution", trail.verticalResolution, allocator);
    value.AddMember("blendingMode", trail.blendingMode, allocator);
    value.AddMember("alwaysOnTop", trail.alwaysOnTop, allocator);
    value.AddMember("renderQueue", trail.renderQueue, allocator);
    value.AddMember("minVertexDistance", trail.minVertexDistance, allocator);
    WriteColorCurve(value, "colorOverAge", trail.colorOverAge, allocator);
    WriteCurve(value, "alphaOverAge", trail.alphaOverAge, allocator);
    WriteCurve(value, "widthOverAge", trail.widthOverAge, allocator);
    value.AddMember("speedWidth", trail.speedWidth, allocator);
    value.AddMember("speedAlpha", trail.speedAlpha, allocator);
    value.AddMember("blurFrames", trail.blurFrames, allocator);
    value.AddMember("useTexture", trail.useTexture, allocator);
    WriteString(value, "texturePath", trail.texturePath, allocator);
    WriteVec2(value, "tiling", trail.tiling, allocator);
    WriteVec2(value, "uvScroll", trail.uvScroll, allocator);
    WriteTransform(value, trail.transform, allocator);

    rapidjson::Value drivers(rapidjson::kArrayType);
    for (const auto& driver : trail.drivers) WriteDriver(drivers, driver, allocator);
    value.AddMember("drivers", drivers, allocator);

    array.PushBack(value, allocator);
}

void WriteSparks(rapidjson::Value& presetValue, const SparkConfig& sparks, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("enabled", sparks.enabled, allocator);
    value.AddMember("count", sparks.count, allocator);
    value.AddMember("size", sparks.size, allocator);
    value.AddMember("minLifetime", sparks.minLifetime, allocator);
    value.AddMember("maxLifetime", sparks.maxLifetime, allocator);
    value.AddMember("velocity", sparks.velocity, allocator);
    value.AddMember("spread", sparks.spread, allocator);
    value.AddMember("trailLength", sparks.trailLength, allocator);
    value.AddMember("trailWidth", sparks.trailWidth, allocator);
    value.AddMember("gravityY", sparks.gravityY, allocator);
    value.AddMember("damping", sparks.damping, allocator);
    value.AddMember("turbulencePower", sparks.turbulencePower, allocator);
    value.AddMember("turbulenceScale", sparks.turbulenceScale, allocator);
    value.AddMember("emissionMode", sparks.emissionMode, allocator);
    value.AddMember("triggerPercentage", sparks.triggerPercentage, allocator);
    value.AddMember("triggerPeriod", sparks.triggerPeriod, allocator);
    value.AddMember("burstSpread", sparks.burstSpread, allocator);
    WriteColor(value, "color", sparks.color, allocator);
    WriteColorCurve(value, "colorOverLifetime", sparks.colorOverLifetime, allocator);
    WriteCurve(value, "alphaOverLifetime", sparks.alphaOverLifetime, allocator);
    WriteCurve(value, "scaleOverLifetime", sparks.scaleOverLifetime, allocator);
    WriteTransform(value, sparks.transform, allocator);
    value.AddMember("useTexture", sparks.useTexture, allocator);
    WriteString(value, "texturePath", sparks.texturePath, allocator);
    presetValue.AddMember("sparks", value, allocator);
}

void WriteGlow(rapidjson::Value& presetValue, const GlowConfig& glow, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("enabled", glow.enabled, allocator);
    value.AddMember("intensity", glow.intensity, allocator);
    value.AddMember("radius", glow.radius, allocator);
    WriteColor(value, "color", glow.color, allocator);
    WriteColorCurve(value, "pulseGradient", glow.pulseGradient, allocator);
    value.AddMember("pulseEnabled", glow.pulseEnabled, allocator);
    value.AddMember("pulseSpeed", glow.pulseSpeed, allocator);
    value.AddMember("pulseAmount", glow.pulseAmount, allocator);
    value.AddMember("useLight", glow.useLight, allocator);
    value.AddMember("lightRange", glow.lightRange, allocator);
    presetValue.AddMember("glow", value, allocator);
}

void WriteBolts(rapidjson::Value& presetValue, const BoltsConfig& bolts, Allocator& allocator) {
    rapidjson::Value value(rapidjson::kObjectType);
    value.AddMember("enabled", bolts.enabled, allocator);
    value.AddMember("joints", bolts.joints, allocator);
    value.AddMember("spacing", bolts.spacing, allocator);
    value.AddMember("size", bolts.size, allocator);
    value.AddMember("stiffness", bolts.stiffness, allocator);
    value.AddMember("damping", bolts.damping, allocator);
    value.AddMember("gravityY", bolts.gravityY, allocator);
    WriteColor(value, "color", bolts.color, allocator);
    presetValue.AddMember("bolts", value, allocator);
}

void WritePreset(rapidjson::Value& object, const SaberPreset& preset, Allocator& allocator) {
    WriteString(object, "id", preset.id, allocator);
    WriteString(object, "name", preset.name, allocator);
    WriteString(object, "author", preset.author, allocator);
    WriteString(object, "description", preset.description, allocator);
    object.AddMember("version", preset.version, allocator);
    WriteTransform(object, preset.transform, allocator);
    WriteBlade(object, preset.blade, allocator);

    rapidjson::Value trails(rapidjson::kArrayType);
    for (const auto& trail : preset.trails) WriteTrail(trails, trail, allocator);
    object.AddMember("trails", trails, allocator);

    WriteSparks(object, preset.sparks, allocator);
    WriteGlow(object, preset.glow, allocator);
    WriteBolts(object, preset.bolts, allocator);
    object.AddMember("keepVanillaModel", preset.keepVanillaModel, allocator);

    rapidjson::Value drivers(rapidjson::kArrayType);
    for (const auto& driver : preset.drivers) WriteDriver(drivers, driver, allocator);
    object.AddMember("drivers", drivers, allocator);
}

void WriteConfig(rapidjson::Value& object, const FreeSaberConfig& config, Allocator& allocator) {
    object.AddMember("formatVersion", config.formatVersion, allocator);
    WriteString(object, "name", config.name, allocator);
    WriteString(object, "author", config.author, allocator);
    WriteString(object, "description", config.description, allocator);
    WriteString(object, "sourceFormat", config.sourceFormat, allocator);
    WriteString(object, "sourceVersion", config.sourceVersion, allocator);
    WriteString(object, "createdDate", config.createdDate, allocator);
    WriteString(object, "modifiedDate", config.modifiedDate, allocator);
    object.AddMember("useSameForBoth", config.useSameForBoth, allocator);

    rapidjson::Value left(rapidjson::kObjectType);
    WritePreset(left, config.leftSaber, allocator);
    object.AddMember("leftSaber", left, allocator);

    rapidjson::Value right(rapidjson::kObjectType);
    WritePreset(right, config.rightSaber, allocator);
    object.AddMember("rightSaber", right, allocator);
}

// ---------------------------------------------------------------------------
// JSON reading
// ---------------------------------------------------------------------------

Curve ReadCurve(const rapidjson::Value& object, const char* key, const Curve& fallback = Curve()) {
    if (const auto* value = Json::Member(object, key)) return Json::CurveValue(*value);
    return fallback;
}

ColorCurve ReadColorCurve(const rapidjson::Value& object, const char* key, const ColorCurve& fallback = ColorCurve()) {
    if (const auto* value = Json::Member(object, key)) return Json::ColorCurveValue(*value);
    return fallback;
}

Color ReadColor(const rapidjson::Value& object, const char* key, const Color& fallback) {
    if (const auto* value = Json::Member(object, key)) return Json::ColorValue(*value, fallback);
    return fallback;
}

void ReadDriver(const rapidjson::Value& value, MaterialDriver& driver) {
    driver.name = Json::String(value, "name", driver.name);
    driver.valueType = Json::Int(value, "valueType", driver.valueType);
    driver.increaseResistance = Json::Number(value, "increaseResistance", driver.increaseResistance);
    driver.decreaseResistance = Json::Number(value, "decreaseResistance", driver.decreaseResistance);
    driver.valueFrom = Json::Number(value, "valueFrom", driver.valueFrom);
    driver.valueTo = Json::Number(value, "valueTo", driver.valueTo);
    driver.valueOverValue = ReadCurve(value, "valueOverValue", driver.valueOverValue);
    driver.colorOverValue = ReadColorCurve(value, "colorOverValue", driver.colorOverValue);
}

std::vector<MaterialDriver> ReadDrivers(const rapidjson::Value& object, const char* key) {
    std::vector<MaterialDriver> drivers;
    const auto* array = Json::Member(object, key);
    if (!array || !array->IsArray()) return drivers;
    for (const auto& item : array->GetArray()) {
        if (!item.IsObject()) continue;
        MaterialDriver driver;
        ReadDriver(item, driver);
        drivers.push_back(driver);
    }
    return drivers;
}

BladeConfig ReadBlade(const rapidjson::Value& object, BladeConfig blade) {
    if (!object.IsObject()) return blade;

    blade.enabled = Json::Bool(object, "enabled", blade.enabled);
    blade.zOffsetFrom = Json::Number(object, "zOffsetFrom", blade.zOffsetFrom);
    blade.zOffsetTo = Json::Number(object, "zOffsetTo", blade.zOffsetTo);
    blade.radius = std::clamp(Json::Number(object, "radius", blade.radius), 0.0005f, 0.35f);
    blade.stretchBias = Json::Number(object, "stretchBias", blade.stretchBias);
    blade.profile = ReadCurve(object, "profile", blade.profile);
    blade.startCap = Json::Bool(object, "startCap", blade.startCap);
    blade.endCap = Json::Bool(object, "endCap", blade.endCap);
    blade.verticalResolution = std::clamp(Json::Int(object, "verticalResolution", blade.verticalResolution), 2, 64);
    blade.horizontalResolution = std::clamp(Json::Int(object, "horizontalResolution", blade.horizontalResolution), 3, 32);

    blade.baseColor = ReadColor(object, "baseColor", blade.baseColor);
    blade.emissionColor = ReadColor(object, "emissionColor", blade.emissionColor);
    blade.emissionIntensity = Json::Number(object, "emissionIntensity", blade.emissionIntensity);
    blade.reflectionColor = ReadColor(object, "reflectionColor", blade.reflectionColor);
    blade.envLightColor = ReadColor(object, "envLightColor", blade.envLightColor);
    blade.fresnelPower = Json::Number(object, "fresnelPower", blade.fresnelPower);
    blade.metallic = Json::Number(object, "metallic", blade.metallic);
    blade.roughness = Json::Number(object, "roughness", blade.roughness);
    blade.envLightIntensity = Json::Number(object, "envLightIntensity", blade.envLightIntensity);
    blade.reflectionIntensity = Json::Number(object, "reflectionIntensity", blade.reflectionIntensity);
    blade.normalMapIntensity = Json::Number(object, "normalMapIntensity", blade.normalMapIntensity);
    blade.sceneReflections = Json::Bool(object, "sceneReflections", blade.sceneReflections);
    blade.sceneLights = Json::Bool(object, "sceneLights", blade.sceneLights);
    blade.glowMultiplier = Json::Number(object, "glowMultiplier", blade.glowMultiplier);
    blade.handleColor = ReadColor(object, "handleColor", blade.handleColor);
    blade.handleRoughness = Json::Number(object, "handleRoughness", blade.handleRoughness);
    blade.renderQueue = Json::Int(object, "renderQueue", blade.renderQueue);
    blade.cullMode = Json::Int(object, "cullMode", blade.cullMode);
    blade.depthWrite = Json::Bool(object, "depthWrite", blade.depthWrite);
    blade.blurFrames = Json::Number(object, "blurFrames", blade.blurFrames);
    blade.blurSensitivity = Json::Number(object, "blurSensitivity", blade.blurSensitivity);

    blade.colorOverValue = ReadColorCurve(object, "colorOverValue", blade.colorOverValue);
    blade.alphaOverValue = ReadCurve(object, "alphaOverValue", blade.alphaOverValue);
    blade.scaleOverValue = ReadCurve(object, "scaleOverValue", blade.scaleOverValue);
    blade.valueFrom = Json::Number(object, "valueFrom", blade.valueFrom);
    blade.valueTo = Json::Number(object, "valueTo", blade.valueTo);
    blade.handleMask = ReadCurve(object, "handleMask", blade.handleMask);
    blade.bladeMaskResolution = Json::Int(object, "bladeMaskResolution", blade.bladeMaskResolution);

    if (const auto* value = Json::Member(object, "tiling")) blade.tiling = Json::Vec2Value(*value, blade.tiling);
    if (const auto* value = Json::Member(object, "uvScroll")) blade.uvScroll = Json::Vec2Value(*value, blade.uvScroll);
    blade.useTexture = Json::Bool(object, "useTexture", blade.useTexture);
    blade.texturePath = Json::String(object, "texturePath", blade.texturePath);

    if (const auto* colorOverride = Json::Member(object, "colorOverride")) {
        blade.colorOverride.force = Json::Bool(*colorOverride, "force", blade.colorOverride.force);
        blade.colorOverride.type = Json::Int(*colorOverride, "type", blade.colorOverride.type);
        blade.colorOverride.hue = Json::Number(*colorOverride, "hue", blade.colorOverride.hue);
        blade.colorOverride.saturation = Json::Number(*colorOverride, "saturation", blade.colorOverride.saturation);
        blade.colorOverride.value = Json::Number(*colorOverride, "value", blade.colorOverride.value);
        blade.colorOverride.hueShiftPerSecond = Json::Number(*colorOverride, "hueShiftPerSecond", blade.colorOverride.hueShiftPerSecond);
        blade.colorOverride.fakeGlowMultiplier = Json::Number(*colorOverride, "fakeGlowMultiplier", blade.colorOverride.fakeGlowMultiplier);
        blade.colorOverride.colorSource = Json::Int(*colorOverride, "colorSource", blade.colorOverride.colorSource);
    }

    return blade;
}

TrailConfig ReadTrail(const rapidjson::Value& object, TrailConfig trail) {
    if (!object.IsObject()) return trail;

    trail.enabled = Json::Bool(object, "enabled", trail.enabled);
    trail.name = Json::String(object, "name", trail.name);
    trail.trailType = Json::Int(object, "trailType", trail.trailType);
    trail.length = std::clamp(Json::Number(object, "length", trail.length), 0.01f, 2.0f);
    trail.width = std::clamp(Json::Number(object, "width", trail.width), 0.0005f, 1.0f);
    trail.offset = Json::Number(object, "offset", trail.offset);
    trail.distortionMultiplier = Json::Number(object, "distortionMultiplier", trail.distortionMultiplier);
    trail.horizontalResolution = std::clamp(Json::Int(object, "horizontalResolution", trail.horizontalResolution), 1, 8);
    trail.verticalResolution = std::clamp(Json::Int(object, "verticalResolution", trail.verticalResolution), 2, 128);
    trail.blendingMode = Json::Int(object, "blendingMode", trail.blendingMode);
    trail.alwaysOnTop = Json::Bool(object, "alwaysOnTop", trail.alwaysOnTop);
    trail.renderQueue = Json::Int(object, "renderQueue", trail.renderQueue);
    trail.minVertexDistance = Json::Number(object, "minVertexDistance", trail.minVertexDistance);
    trail.colorOverAge = ReadColorCurve(object, "colorOverAge", trail.colorOverAge);
    trail.alphaOverAge = ReadCurve(object, "alphaOverAge", trail.alphaOverAge);
    trail.widthOverAge = ReadCurve(object, "widthOverAge", trail.widthOverAge);
    trail.speedWidth = Json::Number(object, "speedWidth", trail.speedWidth);
    trail.speedAlpha = Json::Number(object, "speedAlpha", trail.speedAlpha);
    trail.blurFrames = Json::Number(object, "blurFrames", trail.blurFrames);
    trail.useTexture = Json::Bool(object, "useTexture", trail.useTexture);
    trail.texturePath = Json::String(object, "texturePath", trail.texturePath);
    if (const auto* value = Json::Member(object, "tiling")) trail.tiling = Json::Vec2Value(*value, trail.tiling);
    if (const auto* value = Json::Member(object, "uvScroll")) trail.uvScroll = Json::Vec2Value(*value, trail.uvScroll);
    if (const auto* value = Json::Member(object, "transform")) trail.transform = Json::TransformValue(*value);
    trail.drivers = ReadDrivers(object, "drivers");
    return trail;
}

SparkConfig ReadSparks(const rapidjson::Value& object, SparkConfig sparks) {
    if (!object.IsObject()) return sparks;

    sparks.enabled = Json::Bool(object, "enabled", sparks.enabled);
    sparks.count = std::clamp(Json::Int(object, "count", sparks.count), 1, 1024);
    sparks.size = Json::Number(object, "size", sparks.size);
    sparks.minLifetime = Json::Number(object, "minLifetime", sparks.minLifetime);
    sparks.maxLifetime = Json::Number(object, "maxLifetime", sparks.maxLifetime);
    sparks.velocity = Json::Number(object, "velocity", sparks.velocity);
    sparks.spread = Json::Number(object, "spread", sparks.spread);
    sparks.trailLength = Json::Number(object, "trailLength", sparks.trailLength);
    sparks.trailWidth = Json::Number(object, "trailWidth", sparks.trailWidth);
    sparks.gravityY = Json::Number(object, "gravityY", sparks.gravityY);
    sparks.damping = Json::Number(object, "damping", sparks.damping);
    sparks.turbulencePower = Json::Number(object, "turbulencePower", sparks.turbulencePower);
    sparks.turbulenceScale = Json::Number(object, "turbulenceScale", sparks.turbulenceScale);
    sparks.emissionMode = Json::Int(object, "emissionMode", sparks.emissionMode);
    sparks.triggerPercentage = Json::Number(object, "triggerPercentage", sparks.triggerPercentage);
    sparks.triggerPeriod = Json::Number(object, "triggerPeriod", sparks.triggerPeriod);
    sparks.burstSpread = Json::Number(object, "burstSpread", sparks.burstSpread);
    sparks.color = ReadColor(object, "color", sparks.color);
    sparks.colorOverLifetime = ReadColorCurve(object, "colorOverLifetime", sparks.colorOverLifetime);
    sparks.alphaOverLifetime = ReadCurve(object, "alphaOverLifetime", sparks.alphaOverLifetime);
    sparks.scaleOverLifetime = ReadCurve(object, "scaleOverLifetime", sparks.scaleOverLifetime);
    if (const auto* value = Json::Member(object, "transform")) sparks.transform = Json::TransformValue(*value);
    sparks.useTexture = Json::Bool(object, "useTexture", sparks.useTexture);
    sparks.texturePath = Json::String(object, "texturePath", sparks.texturePath);
    return sparks;
}

GlowConfig ReadGlow(const rapidjson::Value& object, GlowConfig glow) {
    if (!object.IsObject()) return glow;
    glow.enabled = Json::Bool(object, "enabled", glow.enabled);
    glow.intensity = Json::Number(object, "intensity", glow.intensity);
    glow.radius = Json::Number(object, "radius", glow.radius);
    glow.color = ReadColor(object, "color", glow.color);
    glow.pulseGradient = ReadColorCurve(object, "pulseGradient", glow.pulseGradient);
    glow.pulseEnabled = Json::Bool(object, "pulseEnabled", glow.pulseEnabled);
    glow.pulseSpeed = Json::Number(object, "pulseSpeed", glow.pulseSpeed);
    glow.pulseAmount = Json::Number(object, "pulseAmount", glow.pulseAmount);
    glow.useLight = Json::Bool(object, "useLight", glow.useLight);
    glow.lightRange = Json::Number(object, "lightRange", glow.lightRange);
    return glow;
}

BoltsConfig ReadBolts(const rapidjson::Value& object, BoltsConfig bolts) {
    if (!object.IsObject()) return bolts;
    bolts.enabled = Json::Bool(object, "enabled", bolts.enabled);
    bolts.joints = std::clamp(Json::Int(object, "joints", bolts.joints), 1, 12);
    bolts.spacing = Json::Number(object, "spacing", bolts.spacing);
    bolts.size = Json::Number(object, "size", bolts.size);
    bolts.stiffness = Json::Number(object, "stiffness", bolts.stiffness);
    bolts.damping = Json::Number(object, "damping", bolts.damping);
    bolts.gravityY = Json::Number(object, "gravityY", bolts.gravityY);
    bolts.color = ReadColor(object, "color", bolts.color);
    return bolts;
}

SaberPreset ReadPreset(const rapidjson::Value& object, SaberPreset preset) {
    if (!object.IsObject()) return preset;

    preset.id = Json::String(object, "id", preset.id);
    preset.name = Json::String(object, "name", preset.name);
    preset.author = Json::String(object, "author", preset.author);
    preset.description = Json::String(object, "description", preset.description);
    preset.version = Json::Int(object, "version", preset.version);
    if (const auto* value = Json::Member(object, "transform")) preset.transform = Json::TransformValue(*value);
    if (const auto* value = Json::Member(object, "blade")) preset.blade = ReadBlade(*value, preset.blade);

    preset.trails.clear();
    if (const auto* trails = Json::Member(object, "trails")) {
        if (trails->IsArray()) {
            for (const auto& item : trails->GetArray()) {
                if (!item.IsObject()) continue;
                preset.trails.push_back(ReadTrail(item, TrailConfig()));
            }
        }
    }

    if (const auto* value = Json::Member(object, "sparks")) preset.sparks = ReadSparks(*value, preset.sparks);
    if (const auto* value = Json::Member(object, "glow")) preset.glow = ReadGlow(*value, preset.glow);
    if (const auto* value = Json::Member(object, "bolts")) preset.bolts = ReadBolts(*value, preset.bolts);
    preset.keepVanillaModel = Json::Bool(object, "keepVanillaModel", preset.keepVanillaModel);
    preset.drivers = ReadDrivers(object, "drivers");
    return preset;
}

} // namespace

// ============================================================================
// Value helpers
// ============================================================================

Color Color::Lerp(const Color& from, const Color& to, float t) {
    t = Clamp01(t);
    return Color(
        from.r + (to.r - from.r) * t,
        from.g + (to.g - from.g) * t,
        from.b + (to.b - from.b) * t,
        from.a + (to.a - from.a) * t);
}

Color Color::FromHSV(float h, float s, float v, float a) {
    h = h - std::floor(h);
    s = Clamp01(s);
    v = Clamp01(v);

    const float hh = h * 6.0f;
    const int sector = static_cast<int>(std::floor(hh)) % 6;
    const float f = hh - std::floor(hh);
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));

    switch (sector) {
        case 0: return Color(v, t, p, a);
        case 1: return Color(q, v, p, a);
        case 2: return Color(p, v, t, a);
        case 3: return Color(p, q, v, a);
        case 4: return Color(t, p, v, a);
        default: return Color(v, p, q, a);
    }
}

float Curve::Evaluate(float t) const {
    if (points.empty()) return 1.0f;
    if (points.size() == 1) return points.front().value;

    t = Clamp01(t);
    if (t <= points.front().time) return points.front().value;
    if (t >= points.back().time) return points.back().value;

    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const auto& from = points[i];
        const auto& to = points[i + 1];
        if (t < from.time || t > to.time) continue;
        const float span = to.time - from.time;
        float u = span > 1e-6f ? (t - from.time) / span : 0.0f;
        u = ApplyInterpolation(interpolationType, u);
        return from.value + (to.value - from.value) * u;
    }
    return points.back().value;
}

float Curve::EvaluateDriver(float raw) const {
    if (valueTo <= valueFrom) return Evaluate(0.0f);
    return Evaluate((raw - valueFrom) / (valueTo - valueFrom));
}

Color ColorCurve::Evaluate(float t) const {
    if (points.empty()) return Color(1.0f, 1.0f, 1.0f, 1.0f);
    if (points.size() == 1) return points.front().value;

    t = Clamp01(t);
    if (t <= points.front().time) return points.front().value;
    if (t >= points.back().time) return points.back().value;

    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const auto& from = points[i];
        const auto& to = points[i + 1];
        if (t < from.time || t > to.time) continue;
        const float span = to.time - from.time;
        float u = span > 1e-6f ? (t - from.time) / span : 0.0f;
        u = ApplyInterpolation(interpolationType, u);
        return Color::Lerp(from.value, to.value, u);
    }
    return points.back().value;
}

Color ColorCurve::EvaluateDriver(float raw) const {
    if (valueTo <= valueFrom) return Evaluate(0.0f);
    return Evaluate((raw - valueFrom) / (valueTo - valueFrom));
}

bool TransformOffset::IsIdentity() const {
    return position.x == 0.0f && position.y == 0.0f && position.z == 0.0f &&
           rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f &&
           scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f;
}

Color ColorOverride::Evaluate(float time) const {
    if (!force) return Color(1.0f, 1.0f, 1.0f, 1.0f);
    const float animatedHue = hue + hueShiftPerSecond * time;
    return Color::FromHSV(animatedHue, saturation, value, 1.0f);
}

Color BladeConfig::SampleColor(float t) const {
    if (colorOverValue.Empty()) return baseColor;
    return colorOverValue.Evaluate(t);
}

float BladeConfig::SampleAlpha(float t) const {
    const float alpha = alphaOverValue.Empty() ? 1.0f : alphaOverValue.Evaluate(t);
    return Clamp01(alpha);
}

float BladeConfig::SampleScale(float t) const {
    if (scaleOverValue.Empty()) return 1.0f;
    return std::clamp(scaleOverValue.Evaluate(t), 0.01f, 4.0f);
}

float MaterialDriver::EvaluateValue(float raw) const {
    float normalized = 0.0f;
    if (valueTo > valueFrom) {
        normalized = (raw - valueFrom) / (valueTo - valueFrom);
    } else if (valueTo < valueFrom) {
        normalized = (valueFrom - raw) / (valueFrom - valueTo);
    }
    normalized = Clamp01(normalized);
    if (valueOverValue.Empty()) return normalized;
    return Clamp01(valueOverValue.Evaluate(normalized));
}

Color MaterialDriver::EvaluateColor(float raw) const {
    float normalized = 0.0f;
    if (valueTo != valueFrom) {
        normalized = (raw - valueFrom) / (valueTo - valueFrom);
    }
    normalized = Clamp01(normalized);
    if (colorOverValue.Empty()) return Color(1.0f, 1.0f, 1.0f, 1.0f);
    return colorOverValue.Evaluate(normalized);
}

// ============================================================================
// FreeSaberConfig serialization
// ============================================================================

std::string FreeSaberConfig::ToJsonString(bool pretty) const {
    rapidjson::Document document;
    document.SetObject();
    auto& allocator = document.GetAllocator();

    rapidjson::Value root(rapidjson::kObjectType);
    WriteConfig(root, *this, allocator);
    document.AddMember("FreeSaber", root, allocator);

    rapidjson::StringBuffer buffer;
    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        writer.SetIndent(' ', 2);
        document.Accept(writer);
    } else {
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);
    }
    return std::string(buffer.GetString(), buffer.GetSize());
}

bool FreeSaberConfig::FromJsonString(const std::string& jsonStr) {
    rapidjson::Document document;
    document.Parse(jsonStr.c_str(), jsonStr.size());
    if (document.HasParseError() || !document.IsObject()) return false;

    const rapidjson::Value* root = Json::Member(document, "FreeSaber");
    if (!root) root = Json::Member(document, "FreeSaberPreset");
    if (!root) root = &document;
    if (!root->IsObject()) return false;

    formatVersion = Json::Int(*root, "formatVersion", 2);
    name = Json::String(*root, "name", "Untitled Saber");
    author = Json::String(*root, "author", "Unknown");
    description = Json::String(*root, "description", description);
    sourceFormat = Json::String(*root, "sourceFormat", "FreeSabers");
    sourceVersion = Json::String(*root, "sourceVersion", sourceVersion);
    createdDate = Json::String(*root, "createdDate", createdDate);
    modifiedDate = Json::String(*root, "modifiedDate", modifiedDate);
    useSameForBoth = Json::Bool(*root, "useSameForBoth", useSameForBoth);

    SaberPreset left;
    left.name = name;
    left.author = author;
    if (const auto* value = Json::Member(*root, "leftSaber")) left = ReadPreset(*value, left);
    leftSaber = left;

    SaberPreset right = leftSaber;
    if (const auto* value = Json::Member(*root, "rightSaber")) right = ReadPreset(*value, right);
    rightSaber = right;

    // Backwards compatibility with the first .freesaber iteration, which stored
    // effects as a list of layers inside each saber preset.
    auto readLayers = [](const rapidjson::Value& presetValue, SaberPreset& preset) {
        const auto* layers = Json::Member(presetValue, "layers");
        if (!layers || !layers->IsArray()) return;
        for (const auto& layer : layers->GetArray()) {
            if (!layer.IsObject() || !Json::Bool(layer, "enabled", true)) continue;
            const int type = Json::Int(layer, "type", 0);
            switch (type) {
                case 0: { // Blade
                    preset.blade.enabled = true;
                    preset.blade.baseColor = ReadColor(layer, "baseColor", preset.blade.baseColor);
                    preset.blade.emissionColor = ReadColor(layer, "emissionColor", preset.blade.emissionColor);
                    preset.blade.emissionIntensity = Json::Number(layer, "emissionIntensity", preset.blade.emissionIntensity);
                    if (const auto* value = Json::Member(layer, "trail")) {
                        const auto& trailObject = *value;
                        TrailConfig trail = preset.trails.empty() ? TrailConfig() : preset.trails.front();
                        trail.colorOverAge = ColorCurve();
                        trail.colorOverAge.points.push_back({0.0f, ReadColor(trailObject, "colorGradient", Color(0, 0.8f, 1.0f, 1.0f))});
                        trail.length = Json::Number(trailObject, "lifetime", 0.25f);
                        trail.width = Json::Number(trailObject, "width", 0.1f);
                        preset.trails.push_back(trail);
                    }
                    break;
                }
                case 1: { // Trail
                    if (const auto* value = Json::Member(layer, "trail")) {
                        TrailConfig trail;
                        trail.length = Json::Number(*value, "lifetime", 0.3f);
                        trail.width = Json::Number(*value, "width", 0.1f);
                        preset.trails.push_back(trail);
                    }
                    break;
                }
                case 2: { // Glow
                    if (const auto* value = Json::Member(layer, "glow")) preset.glow = ReadGlow(*value, preset.glow);
                    break;
                }
                case 3: { // Sparks
                    if (const auto* value = Json::Member(layer, "sparks")) preset.sparks = ReadSparks(*value, preset.sparks);
                    break;
                }
                case 4: { // Dynamic joints
                    if (const auto* value = Json::Member(layer, "dynamicJoints")) {
                        preset.bolts.enabled = Json::Bool(*value, "enabled", true);
                        preset.bolts.joints = std::clamp(Json::Int(*value, "jointCount", 5), 1, 12);
                        preset.bolts.stiffness = Json::Number(*value, "stiffness", 0.5f) * 120.0f;
                        preset.bolts.damping = Json::Number(*value, "damping", 0.1f) * 80.0f;
                        preset.bolts.gravityY = Json::Number(*value, "gravity", -9.81f) * 0.1f;
                    }
                    break;
                }
                default: break;
            }
        }
    };

    if (const auto* value = Json::Member(*root, "leftSaber")) readLayers(*value, leftSaber);
    if (const auto* value = Json::Member(*root, "rightSaber")) readLayers(*value, rightSaber);

    return true;
}

bool FreeSaberConfig::SaveToFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    FreeSaberConfig copy = *this;
    copy.modifiedDate = NowString();
    if (copy.createdDate.empty()) copy.createdDate = copy.modifiedDate;

    file << copy.ToJsonString(true) << "\n";
    return file.good();
}

std::optional<FreeSaberConfig> FreeSaberConfig::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return std::nullopt;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    FreeSaberConfig config;
    if (!config.FromJsonString(content)) return std::nullopt;
    return config;
}

bool FreeSaberConfig::Validate(std::string& errorOut) const {
    if (name.empty()) {
        errorOut = "Preset name cannot be empty";
        return false;
    }
    if (leftSaber.blade.radius <= 0.0f || rightSaber.blade.radius <= 0.0f) {
        errorOut = "Blade radius must be positive";
        return false;
    }
    if (leftSaber.trails.size() > 8 || rightSaber.trails.size() > 8) {
        errorOut = "Too many trails (maximum 8 per saber)";
        return false;
    }
    return true;
}

const SaberPreset& FreeSaberConfig::GetPreset(bool isLeft) const {
    if (!useSameForBoth && !isLeft) return rightSaber;
    return leftSaber;
}

} // namespace FreeSabers
