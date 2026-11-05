#include "saber/SaberManager.hpp"
#include "config/Config.hpp"
#include "saber/SaberModel.hpp"
#include "saber/TrailRenderer.hpp"
#include "saber/GlowEffect.hpp"
#include "saber/SparkSystem.hpp"
#include "saber/DynamicJoints.hpp"
#include "utils/VisualHelpers.hpp"
#include "main.hpp"

#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/SaberTrail.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace FreeSabers {

SaberManager& SaberManager::GetInstance() {
    static SaberManager instance;
    return instance;
}

SaberManager::~SaberManager() = default;

UnityEngine::Vector3 SaberManager::GetBladePoint(const Instance& instance, float z) const {
    if (instance.blade) return instance.blade->GetPoint(z);
    if (instance.parent) return instance.parent->TransformPoint(UnityEngine::Vector3{0.0f, 0.0f, z});
    return UnityEngine::Vector3{0.0f, 0.0f, z};
}

void SaberManager::Initialize() {
    PaperLogger.info("FreeSabers: saber manager ready (%d preset(s))",
                     static_cast<int>(Config::GetInstance().GetPresets().size()));
}

bool SaberManager::IsEnabled() const {
    return Config::GetInstance().IsEnabled();
}

void SaberManager::OnSaberModelInit(GlobalNamespace::SaberModelController* model, UnityEngine::Transform* parent,
                                    GlobalNamespace::Saber* saber, UnityEngine::Color tint) {
    if (!model || !parent || !saber) return;

    const bool isLeft = saber->get_saberType() == GlobalNamespace::SaberType::SaberA;
    const FreeSaberConfig* active = isLeft
        ? Config::GetInstance().GetActiveLeftPreset()
        : Config::GetInstance().GetActiveRightPreset();

    auto [it, inserted] = sabers.try_emplace(model);
    Instance& instance = it->second;

    if (!inserted && instance.model == model) {
        DisposeVisuals(instance);
    }

    instance.model = model;
    instance.saber = saber;
    instance.parent = parent;
    instance.isLeft = isLeft;
    instance.tint = tint;
    instance.speed = 0.0f;
    instance.time = 0.0f;
    instance.timeSinceCut = 10.0f;

    if (!active) {
        PaperLogger.warn("FreeSabers: no active preset; sabers left untouched");
        return;
    }

    instance.preset = active->GetPreset(isLeft);

    BuildVisuals(instance);
    instance.lastTip = GetBladePoint(instance, instance.preset.blade.zOffsetTo);
    PaperLogger.info("FreeSabers: %s saber using preset '%s'",
                     isLeft ? "left" : "right", instance.preset.name.c_str());
}

void SaberManager::AddImplicitBlurTrail(Instance& instance) {
    const BladeConfig& blade = instance.preset.blade;
    if (blade.blurFrames <= 0.01f) return;

    for (const auto& existing : instance.preset.trails) {
        if (existing.trailType == 1 && existing.enabled) return;
    }

    TrailConfig blur;
    blur.name = "Motion Blur";
    blur.enabled = true;
    blur.trailType = 1;
    blur.length = std::clamp(blade.blurFrames / 72.0f, 0.03f, 0.30f);
    blur.width = blade.radius * 2.0f;
    blur.speedWidth = std::max(blade.blurSensitivity, 0.0f) * 0.22f;
    blur.speedAlpha = 0.5f;
    blur.minVertexDistance = 0.003f;

    const Color tipColor = blade.colorOverValue.Empty() ? blade.baseColor : blade.SampleColor(1.0f);
    blur.colorOverAge.points = {
        {0.0f, tipColor.WithAlpha(1.0f)},
        {0.55f, tipColor.WithAlpha(0.55f)},
        {1.0f, tipColor.WithAlpha(0.0f)},
    };
    blur.alphaOverAge.points = {{0.0f, 1.0f}, {1.0f, 0.0f}};
    blur.widthOverAge.points = {{0.0f, 1.0f}, {1.0f, 0.12f}};

    instance.preset.trails.push_back(blur);
    PaperLogger.info("FreeSabers: generated motion-blur trail (%.2fs) from blurFrames %.2f",
                     blur.length, blade.blurFrames);
}

void SaberManager::BuildVisuals(Instance& instance) {
    if (!instance.parent) return;

    if (instance.preset.trails.empty()) {
        AddImplicitBlurTrail(instance);
    }

    UnityEngine::MeshRenderer* vanillaBlade = nullptr;
    auto* bladeMaterial = Visuals::FindSaberMaterial(instance.parent, false, &vanillaBlade);
    instance.vanillaBladeRenderer = vanillaBlade;

    if (!bladeMaterial) {
        PaperLogger.warn("FreeSabers: could not find a saber material; visual build skipped");
        return;
    }

    if (!instance.preset.keepVanillaModel && instance.preset.blade.enabled) {
        instance.blade = std::make_unique<SaberModel>();
        if (!instance.blade->Initialize(instance.parent, instance.preset.blade, instance.preset.transform,
                                        Visuals::FromUnity(instance.tint), bladeMaterial)) {
            instance.blade.reset();
        }
    }

    auto* trailMaterial = Visuals::FindSaberMaterial(instance.parent, true);
    if (!trailMaterial) trailMaterial = bladeMaterial;

    instance.trails.clear();
    for (const auto& trailConfig : instance.preset.trails) {
        if (!trailConfig.enabled) continue;
        auto trail = std::make_unique<TrailRenderer>();
        if (trail->Initialize(trailConfig, trailMaterial)) {
            instance.trails.push_back(std::move(trail));
        }
    }
    instance.hasTrails = !instance.trails.empty();

    if (instance.preset.glow.enabled) {
        instance.glow = std::make_unique<GlowEffect>();
        if (!instance.glow->Initialize(instance.parent, instance.preset.blade, instance.preset.glow,
                                       instance.preset.transform, Visuals::FromUnity(instance.tint), trailMaterial)) {
            instance.glow.reset();
        }
    }

    if (instance.preset.sparks.enabled) {
        instance.sparks = std::make_unique<SparkSystem>();
        if (!instance.sparks->Initialize(instance.preset.sparks, trailMaterial,
                                         Visuals::FromUnity(instance.tint))) {
            instance.sparks.reset();
        }
    }

    if (instance.preset.bolts.enabled) {
        instance.bolts = std::make_unique<DynamicJoints>();
        if (!instance.bolts->Initialize(instance.preset.bolts, instance.preset.transform,
                                        Visuals::FromUnity(instance.tint), trailMaterial)) {
            instance.bolts.reset();
        }
    }

    HideVanillaModel(instance);
    ApplyVisibility();
}

void SaberManager::HideVanillaModel(Instance& instance) {
    if (!instance.parent) return;

    if (instance.vanillaBladeRenderer && instance.blade && !instance.preset.keepVanillaModel) {
        const std::string name = Visuals::ObjectName(instance.vanillaBladeRenderer->get_gameObject());
        const bool isHilt = name.find("andle") != std::string::npos || name.find("ilt") != std::string::npos;
        if (!isHilt) instance.vanillaBladeRenderer->set_enabled(false);
    }

    instance.vanillaTrailObjects.clear();
    if (!instance.hasTrails) return;

    auto trails = instance.parent->GetComponentsInChildren<GlobalNamespace::SaberTrail*>(true);
    for (GlobalNamespace::SaberTrail* trail : trails) {
        if (!trail) continue;
        UnityEngine::GameObject* object = trail->get_gameObject();
        if (!object || object == instance.parent->get_gameObject()) continue;
        instance.vanillaTrailObjects.push_back(object);
        object->SetActive(false);
    }
}

void SaberManager::DisposeVisuals(Instance& instance) {
    if (Visuals::IsAlive(instance.vanillaBladeRenderer)) {
        instance.vanillaBladeRenderer->set_enabled(true);
    }
    for (auto* trailObject : instance.vanillaTrailObjects) {
        if (Visuals::IsAlive(trailObject)) trailObject->SetActive(true);
    }

    if (instance.blade) instance.blade->Dispose();
    for (auto& trail : instance.trails) {
        if (trail) trail->Dispose();
    }
    if (instance.glow) instance.glow->Dispose();
    if (instance.sparks) instance.sparks->Dispose();
    if (instance.bolts) instance.bolts->Dispose();

    instance.blade.reset();
    instance.trails.clear();
    instance.glow.reset();
    instance.sparks.reset();
    instance.bolts.reset();
    instance.hasTrails = false;
    instance.vanillaBladeRenderer = nullptr;
    instance.vanillaTrailObjects.clear();
}

void SaberManager::ApplyVisibility() {
    const bool enabled = Config::GetInstance().IsEnabled();
    const bool trailsEnabled = Config::GetInstance().AreTrailsEnabled();
    const bool sparksEnabled = Config::GetInstance().AreSparksEnabled();
    const bool glowEnabled = Config::GetInstance().IsGlowEnabled();

    for (auto& [model, instance] : sabers) {
        if (instance.blade) instance.blade->SetVisible(enabled && !instance.preset.keepVanillaModel);
        for (auto& trail : instance.trails) {
            trail->SetVisible(enabled && trailsEnabled);
        }
        if (instance.glow) instance.glow->SetVisible(enabled && glowEnabled);
        if (instance.sparks) instance.sparks->SetVisible(enabled && sparksEnabled);
        if (instance.bolts) instance.bolts->SetVisible(enabled);

        if (instance.vanillaBladeRenderer && Visuals::IsAlive(instance.vanillaBladeRenderer)) {
            instance.vanillaBladeRenderer->set_enabled(
                instance.preset.keepVanillaModel || !enabled || !instance.blade);
        }
        for (auto* trailObject : instance.vanillaTrailObjects) {
            if (Visuals::IsAlive(trailObject)) {
                trailObject->SetActive(!(enabled && trailsEnabled && instance.hasTrails));
            }
        }
    }
}

float SaberManager::EvaluateDriver(int valueType, const Instance& instance, const UnityEngine::Vector3& direction) const {
    switch (static_cast<DriverType>(valueType)) {
        case DriverType::TipVelocity:
        case DriverType::TipAcceleration:
        case DriverType::AngularVelocity:
            return instance.speed;
        case DriverType::SwingsPerSecond:
            return instance.speed * 0.6f;
        case DriverType::TimeDependence:
            return instance.time;
        case DriverType::TimeAfterCut:
            return instance.timeSinceCut;
        case DriverType::DirectionX:
            return direction.x;
        case DriverType::DirectionY:
            return direction.y;
        case DriverType::DirectionZ:
            return direction.z;
        case DriverType::ScorePercentage:
        case DriverType::Accuracy:
            return 0.5f;
        case DriverType::Energy:
            return 0.75f;
        case DriverType::Multiplier:
            return 1.0f;
        case DriverType::Combo:
        case DriverType::CutsPerSecond:
        case DriverType::CutScore:
        case DriverType::PreCutScore:
        case DriverType::PostCutScore:
        case DriverType::Misses:
        case DriverType::None:
        default:
            return 0.0f;
    }
}

void SaberManager::Update(float deltaTime) {
    const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
    const float safeDt = std::max(dt, 1.0f / 240.0f);

    for (auto it = sabers.begin(); it != sabers.end();) {
        Instance& instance = it->second;
        if (!Visuals::IsAlive(instance.parent) || !Visuals::IsAlive(instance.model)) {
            DisposeVisuals(instance);
            it = sabers.erase(it);
            continue;
        }

        instance.time += dt;
        instance.timeSinceCut += dt;

        const BladeConfig& blade = instance.preset.blade;
        const UnityEngine::Vector3 bottom = GetBladePoint(instance, blade.zOffsetFrom);
        const UnityEngine::Vector3 tip = GetBladePoint(instance, blade.zOffsetTo);
        const UnityEngine::Vector3 direction = Visuals::Normalize(Visuals::Sub(tip, bottom));

        const float instantSpeed = std::min(Visuals::Distance(tip, instance.lastTip) / safeDt, 80.0f);
        const float smoothing = std::min(1.0f, dt * 14.0f);
        instance.speed += (instantSpeed - instance.speed) * smoothing;
        instance.lastTip = tip;

        Color tint = Visuals::FromUnity(instance.tint);
        if (blade.colorOverride.force) {
            tint = blade.colorOverride.Evaluate(instance.time);
        }

        if (instance.blade) instance.blade->Update(dt, tint);

        if (instance.glow) {
            instance.glow->Update(dt, tint);
        }

        for (auto& trail : instance.trails) {
            float driverAlpha = 1.0f;
            for (const auto& driver : trail->GetConfig().drivers) {
                const float raw = EvaluateDriver(driver.valueType, instance, direction);
                driverAlpha *= std::clamp(driver.EvaluateValue(raw), 0.0f, 2.0f);
            }
            trail->Update(dt, bottom, tip, instance.speed, driverAlpha);
        }

        if (instance.sparks) {
            instance.sparks->Update(dt, tip, tint, instance.speed);
        }

        if (instance.bolts) {
            instance.bolts->Update(dt, tip, direction);
        }

        ++it;
    }
}

void SaberManager::OnNoteCut(GlobalNamespace::SaberType saberType, float strength) {
    const bool isLeft = saberType == GlobalNamespace::SaberType::SaberA;
    for (auto& [model, instance] : sabers) {
        if (instance.isLeft != isLeft || !Visuals::IsAlive(instance.parent) || !instance.preset.blade.enabled) continue;

        instance.timeSinceCut = 0.0f;
        if (instance.glow) instance.glow->PulseOnCut(strength);

        if (instance.sparks && Config::GetInstance().AreSparksEnabled() && Config::GetInstance().IsEnabled()) {
            const BladeConfig& blade = instance.preset.blade;
            const UnityEngine::Vector3 bottom = GetBladePoint(instance, blade.zOffsetFrom);
            const UnityEngine::Vector3 tip = GetBladePoint(instance, blade.zOffsetTo);
            const UnityEngine::Vector3 direction = Visuals::Normalize(Visuals::Sub(tip, bottom));
            instance.sparks->Emit(tip, direction, strength);
        }
    }
}

void SaberManager::OnLevelStart() {
    // The previous scene may still hold visual roots or vanilla objects to restore.
    for (auto& [model, instance] : sabers) DisposeVisuals(instance);
    sabers.clear();
    PaperLogger.info("FreeSabers: level started, waiting for saber models");
}

void SaberManager::OnLevelEnd() {
    for (auto& [model, instance] : sabers) {
        DisposeVisuals(instance);
    }
    sabers.clear();
    PaperLogger.info("FreeSabers: level ended, visuals cleaned up");
}

void SaberManager::ReloadPresets() {
    Config::GetInstance().ReloadPresets();
    ReapplyPresets();
}

void SaberManager::ReapplyPresets() {
    for (auto& [model, instance] : sabers) {
        if (!instance.saber || !Visuals::IsAlive(instance.model) || !Visuals::IsAlive(instance.parent)) continue;

        const FreeSaberConfig* active = instance.isLeft
            ? Config::GetInstance().GetActiveLeftPreset()
            : Config::GetInstance().GetActiveRightPreset();
        if (!active) continue;

        DisposeVisuals(instance);
        instance.preset = active->GetPreset(instance.isLeft);
        BuildVisuals(instance);
    }
    ApplyVisibility();
}

std::optional<UnityEngine::Color> SaberManager::GetGameplayColor(GlobalNamespace::SaberType saberType) const {
    if (!Config::GetInstance().IsEnabled()) return std::nullopt;

    const bool isLeft = saberType == GlobalNamespace::SaberType::SaberA;
    for (const auto& [model, instance] : sabers) {
        if (instance.isLeft != isLeft) continue;
        if (instance.preset.blade.colorOverride.force) {
            return Visuals::ToUnity(instance.preset.blade.colorOverride.Evaluate(instance.time));
        }
        break;
    }
    return std::nullopt;
}

} // namespace FreeSabers
