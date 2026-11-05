#pragma once

#include "config/SaberConfig.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"

#include <vector>

namespace FreeSabers {

class TrailRenderer {
public:
    TrailRenderer() = default;
    ~TrailRenderer() = default;

    bool Initialize(const TrailConfig& config, UnityEngine::Material* templateMaterial);
    void Update(float deltaTime, const UnityEngine::Vector3& bottom, const UnityEngine::Vector3& tip,
                float speed, float driverAlpha);
    void SetVisible(bool visible);
    void Clear();
    void Dispose();

    bool IsInitialized() const { return initialized; }
    const TrailConfig& GetConfig() const { return config; }

private:
    struct Sample {
        UnityEngine::Vector3 bottom{};
        UnityEngine::Vector3 tip{};
        float time = 0.0f;
        float speed = 0.0f;
    };

    TrailConfig config;
    std::vector<Sample> samples;
    UnityEngine::GameObject* root = nullptr;
    UnityEngine::Mesh* mesh = nullptr;
    UnityEngine::Material* material = nullptr;
    UnityEngine::MeshRenderer* renderer = nullptr;
    bool initialized = false;

    ArrayW<UnityEngine::Vector3> vertices;
    ArrayW<UnityEngine::Color> colors;
    ArrayW<UnityEngine::Vector2> uvs;
    ArrayW<int> triangles;

    int maxSamples = 32;
    float elapsed = 0.0f;
    float lastSampleTime = 0.0f;
};

} // namespace FreeSabers
