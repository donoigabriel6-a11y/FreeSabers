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

#include <cstdint>
#include <vector>

namespace FreeSabers {

class SparkSystem {
public:
    SparkSystem() = default;
    ~SparkSystem() = default;

    bool Initialize(const SparkConfig& config, UnityEngine::Material* templateMaterial, const Color& tint);
    void Update(float deltaTime, const UnityEngine::Vector3& tip, const Color& tint, float speed);
    void Emit(const UnityEngine::Vector3& position, const UnityEngine::Vector3& direction, float strength);
    void SetVisible(bool visible);
    void Dispose();

    bool IsInitialized() const { return initialized; }

private:
    struct Particle {
        UnityEngine::Vector3 position{};
        UnityEngine::Vector3 velocity{};
        float age = 0.0f;
        float lifetime = 0.3f;
        float seed = 0.0f;
        bool alive = false;
    };

    SparkConfig config;
    Color tintColor = Color();
    std::vector<Particle> particles;
    UnityEngine::GameObject* root = nullptr;
    UnityEngine::Mesh* mesh = nullptr;
    UnityEngine::Material* material = nullptr;
    UnityEngine::MeshRenderer* renderer = nullptr;
    bool initialized = false;

    ArrayW<UnityEngine::Vector3> vertices;
    ArrayW<UnityEngine::Color> colors;
    ArrayW<UnityEngine::Vector2> uvs;
    ArrayW<int> triangles;

    int capacity = 0;
    float elapsed = 0.0f;
    float emitTimer = 0.0f;
    uint32_t rngState = 0x9E3779B9u;

    float Random01();
    Particle* AcquireParticle();
    void WriteBuffers(const UnityEngine::Vector3& cameraRight, const UnityEngine::Vector3& cameraUp);
};

} // namespace FreeSabers
