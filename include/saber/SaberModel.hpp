#pragma once

#include "config/SaberConfig.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Transform.hpp"

namespace FreeSabers {

class SaberModel {
public:
    SaberModel() = default;
    ~SaberModel() = default;

    bool Initialize(UnityEngine::Transform* parent, const BladeConfig& blade, const TransformOffset& offset,
                    const Color& tint, UnityEngine::Material* templateMaterial);

    void Update(float deltaTime, const Color& tint, float driverValue);
    // World-space point along the blade (local Z is transformed by the preset offset).
    UnityEngine::Vector3 GetPoint(float z) const;
    void SetVisible(bool visible);
    void Dispose();

    bool IsInitialized() const { return initialized; }
    const BladeConfig& GetConfig() const { return config; }

private:
    BladeConfig config;
    UnityEngine::GameObject* root = nullptr;
    UnityEngine::Mesh* mesh = nullptr;
    UnityEngine::Material* material = nullptr;
    UnityEngine::MeshRenderer* renderer = nullptr;
    bool initialized = false;
    float time = 0.0f;

    void ApplyMaterial(const Color& tint);
    void ApplyDrivers(const std::vector<MaterialDriver>& drivers, float rawValue, const Color& baseColor);
};

} // namespace FreeSabers
