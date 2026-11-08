#pragma once

#include "config/SaberConfig.hpp"
#include "GlobalNamespace/SaberModelController.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/SaberType.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace FreeSabers {

class SaberModel;
class TrailRenderer;
class GlowEffect;
class SparkSystem;
class DynamicJoints;

class SaberManager {
public:
    static SaberManager& GetInstance();

    void Initialize();

    // Hooks
    void OnSaberModelInit(GlobalNamespace::SaberModelController* model, UnityEngine::Transform* parent,
                          GlobalNamespace::Saber* saber, UnityEngine::Color tint);
    void OnLevelStart();
    void OnLevelEnd();
    void OnNoteCut(GlobalNamespace::SaberType saberType, const GlobalNamespace::NoteCutInfo& noteCutInfo);
    void OnNoteMissed();
    void Update(float deltaTime);

    // Presets / UI
    void ReloadPresets();
    void ReapplyPresets();
    void ApplyVisibility();

    std::optional<UnityEngine::Color> GetGameplayColor(GlobalNamespace::SaberType saberType) const;

    bool IsEnabled() const;
    size_t GetSaberCount() const { return sabers.size(); }

    ~SaberManager();

public:
    // Global gameplay state updates (called from hooks)
    static void SetGlobalMultiplier(int multiplier);
    static void IncrementMisses();
    static void UpdateCutsPerSecond(float cps);
    static void UpdateAccuracy(float accuracy);

private:
    SaberManager() = default;
    SaberManager(const SaberManager&) = delete;
    SaberManager& operator=(const SaberManager&) = delete;

    struct Instance {
        GlobalNamespace::SaberModelController* model = nullptr;
        GlobalNamespace::Saber* saber = nullptr;
        UnityEngine::Transform* parent = nullptr;
        UnityEngine::MeshRenderer* vanillaBladeRenderer = nullptr;
        std::vector<UnityEngine::GameObject*> vanillaTrailObjects;
        SaberPreset preset;
        bool isLeft = false;
        bool hasTrails = false;
        UnityEngine::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
        float speed = 0.0f;
        float time = 0.0f;
        float timeSinceCut = 10.0f;
        UnityEngine::Vector3 lastTip{};

        // Gameplay state for drivers
        int combo = 0;
        int misses = 0;
        int multiplier = 1;
        float energy = 1.0f;
        float scorePercentage = 0.5f;
        float accuracy = 0.5f;
        float cutsPerSecond = 0.0f;
        int cutScore = 0;
        int preCutScore = 0;
        int postCutScore = 0;

        std::unique_ptr<SaberModel> blade;
        std::vector<std::unique_ptr<TrailRenderer>> trails;
        std::unique_ptr<GlowEffect> glow;
        std::unique_ptr<SparkSystem> sparks;
        std::unique_ptr<DynamicJoints> bolts;
    };

    std::unordered_map<GlobalNamespace::SaberModelController*, Instance> sabers;

    void BuildVisuals(Instance& instance);
    void DisposeVisuals(Instance& instance);
    void HideVanillaModel(Instance& instance);
    void AddImplicitBlurTrail(Instance& instance);
    void UpdateGameplayState();
    UnityEngine::Vector3 GetBladePoint(const Instance& instance, float z) const;
    float EvaluateDriver(int valueType, const Instance& instance, const UnityEngine::Vector3& direction) const;
};

} // namespace FreeSabers
