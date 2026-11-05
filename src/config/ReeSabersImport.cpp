#include "config/ReeSabersImport.hpp"
#include "config/JsonHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <rapidjson/document.h>

namespace FreeSabers::ReeSabers {

namespace {

void ParseDrivers(const rapidjson::Value& array, std::vector<MaterialDriver>& out) {
    if (!array.IsArray()) return;

    for (const auto& item : array.GetArray()) {
        if (!item.IsObject()) continue;

        MaterialDriver driver;
        driver.valueType = Json::Int(item, "valueType", 0);
        if (driver.valueType == 0) continue; // unused driver slot

        driver.increaseResistance = Json::Number(item, "increaseResistance", 0.0f);
        driver.decreaseResistance = Json::Number(item, "decreaseResistance", 0.0f);

        if (const auto* mappings = Json::Member(item, "mappings")) {
            if (const auto* value = Json::Member(*mappings, "colorOverValue")) {
                driver.colorOverValue = Json::ColorCurveValue(*value);
            }
            if (const auto* value = Json::Member(*mappings, "alphaOverValue")) {
                driver.valueOverValue = Json::CurveValue(*value);
            } else if (const auto* scale = Json::Member(*mappings, "scaleOverValue")) {
                driver.valueOverValue = Json::CurveValue(*scale);
            }
            driver.valueFrom = Json::Number(*mappings, "valueFrom", 0.0f);
            driver.valueTo = Json::Number(*mappings, "valueTo", 1.0f);
        }
        out.push_back(driver);
    }
}

void ParseBlade(const rapidjson::Value& config, SaberPreset& preset, std::vector<std::string>& warnings) {
    BladeConfig& blade = preset.blade;
    blade.enabled = true;

    const rapidjson::Value* mesh = Json::Member(config, "MeshSettings");
    if (!mesh) mesh = Json::Member(config, "SaberSettings");

    if (mesh) {
        blade.zOffsetFrom = Json::Number(*mesh, "zOffsetFrom", blade.zOffsetFrom);
        blade.zOffsetTo = Json::Number(*mesh, "zOffsetTo", blade.zOffsetTo);

        if (Json::Member(*mesh, "radius")) {
            blade.radius = Json::Number(*mesh, "radius", blade.radius);
        } else if (Json::Member(*mesh, "thickness")) {
            // v1 stored a multiplier of the default saber radius.
            blade.radius = Json::Number(*mesh, "thickness", 1.0f) * 0.0132f;
        }
        blade.radius = std::clamp(blade.radius, 0.001f, 0.2f);
        blade.stretchBias = Json::Number(*mesh, "stretchBias", blade.stretchBias);

        if (const auto* profile = Json::Member(*mesh, "saberProfile")) {
            blade.profile = Json::CurveValue(*profile);
        }

        blade.startCap = Json::Bool(*mesh, "startCap", blade.startCap);
        blade.endCap = Json::Bool(*mesh, "endCap", blade.endCap);
        blade.verticalResolution = std::clamp(Json::Int(*mesh, "verticalResolution", blade.verticalResolution), 2, 48);
        blade.horizontalResolution = std::clamp(Json::Int(*mesh, "horizontalResolution", blade.horizontalResolution), 3, 24);
        blade.renderQueue = Json::Int(*mesh, "renderQueue", blade.renderQueue);
        blade.cullMode = Json::Int(*mesh, "cullMode", blade.cullMode);
        blade.depthWrite = Json::Bool(*mesh, "depthWrite", blade.depthWrite);
        blade.blurFrames = Json::Number(*mesh, "blurFrames", blade.blurFrames);
        blade.glowMultiplier = Json::Number(*mesh, "glowMultiplier", blade.glowMultiplier);
        blade.handleRoughness = Json::Number(*mesh, "handleRoughness", blade.handleRoughness);
        if (const auto* handleColor = Json::Member(*mesh, "handleColor")) {
            blade.handleColor = Json::ColorValue(*handleColor, blade.handleColor);
        }
    }

    const rapidjson::Value* material = Json::Member(config, "MaterialSettings");
    const rapidjson::Value& materialOrMesh = material ? *material : (mesh ? *mesh : config);

    blade.blurFrames = Json::Number(materialOrMesh, "blurFrames", blade.blurFrames);
    blade.blurSensitivity = Json::Number(materialOrMesh, "blurSensitivity", blade.blurSensitivity);
    blade.glowMultiplier = Json::Number(materialOrMesh, "glowMultiplier", blade.glowMultiplier);

    if (const auto* value = Json::Member(materialOrMesh, "reflectionColor")) {
        blade.reflectionColor = Json::ColorValue(*value, blade.reflectionColor);
    }
    if (const auto* value = Json::Member(materialOrMesh, "envLightColor")) {
        blade.envLightColor = Json::ColorValue(*value, blade.envLightColor);
    }
    blade.fresnelPower = Json::Number(materialOrMesh, "fresnelPower", blade.fresnelPower);
    blade.metallic = Json::Number(materialOrMesh, "metallic", blade.metallic);
    blade.roughness = Json::Number(materialOrMesh, "roughness", blade.roughness);
    blade.envLightIntensity = Json::Number(materialOrMesh, "envLightIntensity", blade.envLightIntensity);
    blade.reflectionIntensity = Json::Number(materialOrMesh, "reflectionIntensity", blade.reflectionIntensity);
    blade.normalMapIntensity = Json::Number(materialOrMesh, "normalMapIntensity", blade.normalMapIntensity);
    blade.sceneReflections = Json::Bool(materialOrMesh, "sceneReflections", blade.sceneReflections);
    blade.sceneLights = Json::Bool(materialOrMesh, "sceneLights", blade.sceneLights);
    blade.renderQueue = Json::Int(materialOrMesh, "renderQueue", blade.renderQueue);
    blade.cullMode = Json::Int(materialOrMesh, "cullMode", blade.cullMode);
    blade.depthWrite = Json::Bool(materialOrMesh, "depthWrite", blade.depthWrite);

    const rapidjson::Value* mask = material ? Json::Member(*material, "maskSettings") : nullptr;
    if (!mask && mesh) mask = Json::Member(*mesh, "maskSettings");
    if (mask) {
        if (const auto* mappings = Json::Member(*mask, "bladeMappings")) {
            if (const auto* value = Json::Member(*mappings, "colorOverValue")) {
                blade.colorOverValue = Json::ColorCurveValue(*value);
            }
            if (const auto* value = Json::Member(*mappings, "alphaOverValue")) {
                blade.alphaOverValue = Json::CurveValue(*value);
            }
            if (const auto* value = Json::Member(*mappings, "scaleOverValue")) {
                blade.scaleOverValue = Json::CurveValue(*value);
            }
            blade.valueFrom = Json::Number(*mappings, "valueFrom", 0.0f);
            blade.valueTo = Json::Number(*mappings, "valueTo", 1.0f);
        }
        if (const auto* value = Json::Member(*mask, "handleMask")) {
            blade.handleMask = Json::CurveValue(*value);
        }
        blade.bladeMaskResolution = Json::Int(*mask, "bladeMaskResolution", blade.bladeMaskResolution);
        if (const auto* drivers = Json::Member(*mask, "drivers")) {
            ParseDrivers(*drivers, preset.drivers);
        }
    }

    if (const auto* colorOverride = Json::Member(config, "ColorOverride")) {
        blade.colorOverride.type = Json::Int(*colorOverride, "type", 0);
        blade.colorOverride.hue = Json::Number(*colorOverride, "hue", 0.0f);
        blade.colorOverride.saturation = Json::Number(*colorOverride, "saturation", 1.0f);
        blade.colorOverride.value = Json::Number(*colorOverride, "value", 1.0f);
        blade.colorOverride.hueShiftPerSecond = Json::Number(*colorOverride, "hueShiftPerSecond", 0.0f);
        blade.colorOverride.fakeGlowMultiplier = Json::Number(*colorOverride, "fakeGlowMultiplier", 1.0f);
        blade.colorOverride.colorSource = Json::Int(*colorOverride, "colorSource", 0);
    }
    blade.colorOverride.force = Json::Bool(config, "ForceColorOverride", blade.colorOverride.force);

    if (const auto* textures = Json::Member(config, "TexturesSettings")) {
        if (const auto* tiling = Json::Member(*textures, "tilingLayout")) {
            blade.tiling = Json::Vec2Value(*tiling, blade.tiling);
        }
        if (const auto* scroll = Json::Member(*textures, "uvScroll")) {
            blade.uvScroll = Json::Vec2Value(*scroll, blade.uvScroll);
        }
    }

    if (!blade.colorOverValue.Empty()) {
        blade.baseColor = blade.colorOverValue.Evaluate(1.0f);
    } else if (blade.envLightColor.a > 0.0f) {
        blade.baseColor = blade.envLightColor;
    }
    blade.emissionColor = blade.envLightColor;
    if (blade.emissionIntensity <= 0.0f) blade.emissionIntensity = 1.5f;

    if (blade.useTexture) {
        warnings.push_back("Blade textures from BinaryAssets are skipped; colours from the preset are used instead.");
    }
}

void ParseTrail(const rapidjson::Value& config, SaberPreset& preset, std::vector<std::string>& warnings) {
    TrailConfig trail;
    trail.enabled = true;
    trail.name = Json::String(config, "Name", "Trail");

    if (const auto* mesh = Json::Member(config, "MeshSettings")) {
        trail.length = Json::Number(*mesh, "TrailLength", trail.length);
        trail.horizontalResolution = Json::Int(*mesh, "HorizontalResolution", trail.horizontalResolution);
        trail.verticalResolution = Json::Int(*mesh, "VerticalResolution", trail.verticalResolution);
    }

    if (const auto* material = Json::Member(config, "MaterialSettings")) {
        trail.trailType = Json::Int(*material, "trailType", trail.trailType);
        trail.width = Json::Number(*material, "width", trail.width);
        // Older exporters wrote 1.0 for "default width".
        if (trail.width > 0.5f) trail.width *= 0.02f;
        trail.offset = Json::Number(*material, "offset", trail.offset);
        trail.distortionMultiplier = Json::Number(*material, "distortionMultiplier", trail.distortionMultiplier);

        if (const auto* general = Json::Member(*material, "generalSettings")) {
            trail.blendingMode = Json::Int(*general, "blendingMode", trail.blendingMode);
            trail.alwaysOnTop = Json::Bool(*general, "alwaysOnTop", trail.alwaysOnTop);
            trail.renderQueue = Json::Int(*general, "renderQueue", trail.renderQueue);
            if (const auto* value = Json::Member(*general, "tilingLayout")) {
                trail.tiling = Json::Vec2Value(*value, trail.tiling);
            }
            if (const auto* value = Json::Member(*general, "uvScroll")) {
                trail.uvScroll = Json::Vec2Value(*value, trail.uvScroll);
            }
            const std::string textureId = Json::String(*general, "customTextureId", "");
            if (!textureId.empty()) {
                trail.useTexture = true;
                trail.texturePath = textureId;
                warnings.push_back("Trail texture '" + textureId + "' is skipped (binary assets are not imported).");
            }
        }

        if (const auto* mask = Json::Member(*material, "maskSettings")) {
            if (const auto* mappings = Json::Member(*mask, "lengthMappings")) {
                if (const auto* value = Json::Member(*mappings, "colorOverValue")) {
                    trail.colorOverAge = Json::ColorCurveValue(*value);
                }
                if (const auto* value = Json::Member(*mappings, "alphaOverValue")) {
                    trail.alphaOverAge = Json::CurveValue(*value);
                }
            }
            if (const auto* mappings = Json::Member(*mask, "widthMappings")) {
                if (const auto* value = Json::Member(*mappings, "scaleOverValue")) {
                    trail.widthOverAge = Json::CurveValue(*value);
                }
            }
            if (const auto* drivers = Json::Member(*mask, "drivers")) {
                ParseDrivers(*drivers, trail.drivers);
            }
        }
    }

    if (const auto* transform = Json::Member(config, "LocalTransform")) {
        trail.transform = Json::TransformValue(*transform);
    }

    trail.length = std::clamp(trail.length, 0.02f, 1.0f);
    trail.width = std::clamp(trail.width, 0.001f, 0.5f);
    trail.verticalResolution = std::clamp(trail.verticalResolution, 2, 128);

    // Guarantee a visible trail even if the preset omitted colours.
    if (trail.colorOverAge.Empty()) {
        trail.colorOverAge.points.push_back({0.0f, Color::FromHSV(0.55f, 0.75f, 1.0f, 1.0f)});
        trail.colorOverAge.points.push_back({1.0f, Color::FromHSV(0.62f, 1.0f, 0.35f, 0.0f)});
    }
    if (trail.alphaOverAge.Empty()) {
        trail.alphaOverAge.points.push_back({0.0f, 1.0f});
        trail.alphaOverAge.points.push_back({1.0f, 0.0f});
    }
    if (trail.widthOverAge.Empty()) {
        trail.widthOverAge.points.push_back({0.0f, 1.0f});
        trail.widthOverAge.points.push_back({1.0f, 0.3f});
    }

    preset.trails.push_back(trail);
}

void ParseSparks(const rapidjson::Value& config, SaberPreset& preset, std::vector<std::string>& warnings) {
    SparkConfig sparks;
    sparks.enabled = true;

    if (const auto* mesh = Json::Member(config, "MeshSettings")) {
        sparks.count = std::clamp(Json::Int(*mesh, "ParticlesCount", sparks.count), 8, 1024);
    }

    if (const auto* spawn = Json::Member(config, "SpawnSettings")) {
        sparks.minLifetime = Json::Number(*spawn, "MinLifetime", sparks.minLifetime);
        sparks.maxLifetime = Json::Number(*spawn, "MaxLifetime", sparks.maxLifetime);
        sparks.triggerPeriod = Json::Number(*spawn, "SpawningPeriod", sparks.triggerPeriod);
        const float randomVelocity = Json::Number(*spawn, "RandomSpawnVelocity", 0.0f);
        if (randomVelocity > 0.01f) sparks.velocity = randomVelocity;
    }

    if (const auto* material = Json::Member(config, "MaterialSettings")) {
        sparks.trailWidth = Json::Number(*material, "trailWidth", sparks.trailWidth);
        if (const auto* mask = Json::Member(*material, "maskSettings")) {
            if (const auto* mappings = Json::Member(*mask, "lifetimeMappings")) {
                if (const auto* value = Json::Member(*mappings, "colorOverValue")) {
                    sparks.colorOverLifetime = Json::ColorCurveValue(*value);
                }
                if (const auto* value = Json::Member(*mappings, "alphaOverValue")) {
                    sparks.alphaOverLifetime = Json::CurveValue(*value);
                }
                if (const auto* value = Json::Member(*mappings, "scaleOverValue")) {
                    sparks.scaleOverLifetime = Json::CurveValue(*value);
                }
            }
            if (const auto* drivers = Json::Member(*mask, "drivers")) {
                ParseDrivers(*drivers, preset.drivers);
            }
        }
    }

    if (const auto* trail = Json::Member(config, "TrailSettings")) {
        sparks.trailLength = Json::Number(*trail, "Length", sparks.trailLength);
    }

    if (const auto* trigger = Json::Member(config, "TriggerSettings")) {
        const int type = Json::Int(*trigger, "type", 1);
        sparks.emissionMode = type == 2 ? 2 : (type == 0 ? 0 : 1);
        sparks.triggerPercentage = Json::Number(*trigger, "percentage", sparks.triggerPercentage);
        sparks.triggerPeriod = Json::Number(*trigger, "period", sparks.triggerPeriod);
        sparks.burstSpread = Json::Number(*trigger, "burstSpread", sparks.burstSpread);
    }

    if (const auto* physics = Json::Member(config, "PhysicsSettings")) {
        sparks.damping = Json::Number(*physics, "DampingCoefficient", sparks.damping);
        sparks.turbulenceScale = Json::Number(*physics, "TurbulenceScale", sparks.turbulenceScale);
        sparks.turbulencePower = Json::Number(*physics, "TurbulencePower", sparks.turbulencePower);
        if (const auto* gravity = Json::Member(*physics, "Gravity")) {
            sparks.gravityY = Json::VectorValue(*gravity, Vector3(0.0f, sparks.gravityY, 0.0f)).y;
        }
    }

    if (const auto* transform = Json::Member(config, "LocalTransform")) {
        sparks.transform = Json::TransformValue(*transform);
    }

    sparks.minLifetime = std::clamp(sparks.minLifetime, 0.02f, 2.0f);
    sparks.maxLifetime = std::clamp(sparks.maxLifetime, sparks.minLifetime, 3.0f);
    sparks.size = std::clamp(sparks.size, 0.005f, 0.15f);
    sparks.velocity = std::clamp(sparks.velocity, 0.1f, 12.0f);
    sparks.trailLength = std::clamp(sparks.trailLength, 0.0f, 0.6f);
    sparks.trailWidth = std::clamp(sparks.trailWidth, 0.001f, 0.2f);

    if (!sparks.colorOverLifetime.Empty()) {
        sparks.color = sparks.colorOverLifetime.Evaluate(0.0f);
        sparks.color.a = 1.0f;
    }
    if (sparks.count > 256) {
        warnings.push_back("Sparks count was capped to 256 for Quest performance.");
        sparks.count = 256;
    }

    preset.sparks = sparks;
}

void ParseVanillaSaber(const rapidjson::Value& config, SaberPreset& preset) {
    preset.keepVanillaModel = true;
    preset.blade.enabled = false;

    if (Json::Bool(config, "WithTrail", false) && preset.trails.empty()) {
        TrailConfig trail;
        trail.name = "Vanilla Trail";
        trail.trailType = 1;
        trail.length = 0.25f;
        trail.width = 0.03f;
        trail.colorOverAge.points.push_back({0.0f, Color(1.0f, 1.0f, 1.0f, 1.0f)});
        trail.colorOverAge.points.push_back({1.0f, Color(0.0f, 0.4f, 0.8f, 0.0f)});
        trail.alphaOverAge.points.push_back({0.0f, 1.0f});
        trail.alphaOverAge.points.push_back({1.0f, 0.0f});
        trail.widthOverAge.points.push_back({0.0f, 1.0f});
        trail.widthOverAge.points.push_back({1.0f, 0.15f});
        preset.trails.push_back(trail);
    }
}

} // namespace

ImportResult Import(const std::string& json, const std::string& fallbackName) {
    ImportResult result;

    rapidjson::Document document;
    document.Parse(json.c_str(), json.size());
    if (document.HasParseError()) {
        result.error = "invalid JSON";
        return result;
    }
    if (!document.IsObject()) {
        result.error = "preset root is not an object";
        return result;
    }

    const auto* modules = Json::Member(document, "Modules");
    if (!modules || !modules->IsArray()) {
        result.error = "not a ReeSabers preset (missing Modules array)";
        return result;
    }

    FreeSaberConfig config;
    config.name = fallbackName.empty() ? "ReeSabers Preset" : fallbackName;
    config.author = "ReeSabers";
    config.sourceFormat = "ReeSabers";
    config.sourceVersion = Json::String(document, "ModVersion", "");
    config.useSameForBoth = true;

    SaberPreset preset;
    preset.name = config.name;
    preset.author = config.author;
    preset.blade.enabled = false;
    preset.sparks.enabled = false;

    if (const auto* transform = Json::Member(document, "LocalTransform")) {
        preset.transform = Json::TransformValue(*transform);
    }

    int enabledModules = 0;
    int trailModules = 0;
    int sparkModules = 0;

    std::function<void(const rapidjson::Value&, int)> walk = [&](const rapidjson::Value& list, int depth) {
        if (depth > 16 || !list.IsArray()) return;

        for (const auto& module : list.GetArray()) {
            if (!module.IsObject()) continue;

            const std::string id = Json::String(module, "ModuleId", "");
            const auto* moduleConfig = Json::Member(module, "Config");
            const bool moduleEnabled = moduleConfig ? Json::Bool(*moduleConfig, "Enabled", true) : true;

            if (moduleConfig && moduleEnabled) {
                if (id == "reezonate.blur-saber") {
                    ParseBlade(*moduleConfig, preset, result.warnings);
                    if (const auto* transform = Json::Member(*moduleConfig, "LocalTransform")) {
                        preset.transform = Json::TransformValue(*transform);
                    }
                    ++enabledModules;
                } else if (id == "reezonate.simple-trail") {
                    ParseTrail(*moduleConfig, preset, result.warnings);
                    ++trailModules;
                    ++enabledModules;
                } else if (id == "reezonate.sparks-vfx") {
                    ParseSparks(*moduleConfig, preset, result.warnings);
                    ++sparkModules;
                    ++enabledModules;
                } else if (id == "reezonate.vanilla-saber") {
                    ParseVanillaSaber(*moduleConfig, preset);
                    ++enabledModules;
                } else if (id == "reezonate.empty") {
                    ++enabledModules;
                } else if (!id.empty()) {
                    result.warnings.push_back("Module '" + id + "' is not supported and was skipped.");
                }
            }

            if (const auto* children = Json::Member(module, "Children")) {
                walk(*children, depth + 1);
            }
        }
    };

    walk(*modules, 0);

    if (enabledModules == 0) {
        result.error = "the preset has no enabled modules";
        return result;
    }

    if (preset.trails.size() > 8) {
        preset.trails.resize(8);
        result.warnings.push_back("Only the first 8 trails are imported.");
    }

    // A preset that only has effect modules still needs the game's saber model.
    if (!preset.blade.enabled && !preset.keepVanillaModel && trailModules == 0 && sparkModules == 0) {
        preset.keepVanillaModel = true;
    }

    if (preset.blade.enabled) {
        preset.glow.enabled = true;
        preset.glow.color = preset.blade.SampleColor(1.0f);
        preset.glow.intensity = std::clamp(preset.blade.glowMultiplier, 0.25f, 4.0f);
        preset.glow.radius = std::clamp(0.35f + preset.blade.radius * 18.0f, 0.25f, 1.5f);
    } else {
        preset.glow.enabled = false;
    }

    std::string summary = "Imported from ReeSabers";
    if (!config.sourceVersion.empty()) summary += " " + config.sourceVersion;
    summary += " - " + std::to_string(trailModules) + " trail(s)";
    if (preset.blade.enabled) summary += ", custom blade";
    if (sparkModules > 0) summary += ", sparks";
    config.description = summary;

    preset.name = config.name;
    config.leftSaber = preset;
    config.rightSaber = preset;

    result.config = std::move(config);
    result.success = true;
    return result;
}

bool LooksLikeReeSabers(const std::string& json) {
    rapidjson::Document document;
    document.Parse(json.c_str(), json.size());
    if (document.HasParseError() || !document.IsObject()) return false;
    const auto* modules = Json::Member(document, "Modules");
    return modules && modules->IsArray();
}

std::string ImportResult::Summary() const {
    if (!success) {
        return "ReeSabers import failed: " + error;
    }
    std::string summary = "Imported " + config.name;
    if (!config.sourceVersion.empty()) summary += " (ReeSabers " + config.sourceVersion + ")";
    if (!warnings.empty()) summary += " with " + std::to_string(warnings.size()) + " warning(s)";
    return summary;
}

} // namespace FreeSabers::ReeSabers
