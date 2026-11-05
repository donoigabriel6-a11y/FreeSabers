#pragma once

#include "config/SaberConfig.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Light.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Transform.hpp"

namespace FreeSabers {

class GlowEffect {
public:
    GlowEffect() = default;
    ~GlowEffect() = default;

    bool Initialize(UnityEngine::Transform* parent, const BladeConfig& blade, const GlowConfig& glow,
                    const TransformOffset& offset, const Color& tint, UnityEngine::Material* templateMaterial);

    void Update(float deltaTime, const Color& tint);
    void PulseOnCut(float strength);
    void SetVisible(bool visible);
    void Dispose();

    bool IsInitialized() const { return initialized; }

private:
    GlowConfig config;
    BladeConfig bladeConfig;
    UnityEngine::GameObject* root = nullptr;
    UnityEngine::GameObject* lightObject = nullptr;
    UnityEngine::Mesh* mesh = nullptr;
    UnityEngine::Material* material = nullptr;
    UnityEngine::MeshRenderer* renderer = nullptr;
    UnityEngine::Light* light = nullptr;
    bool initialized = false;
    float time = 0.0f;
    float cutPulse = 0.0f;
    float baseAlpha = 0.22f;
};

} // namespace FreeSabers
