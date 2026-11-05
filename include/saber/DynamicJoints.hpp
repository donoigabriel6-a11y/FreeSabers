#pragma once

#include "config/SaberConfig.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"

#include <vector>

namespace FreeSabers {

class DynamicJoints {
public:
    DynamicJoints() = default;
    ~DynamicJoints() = default;

    bool Initialize(const BoltsConfig& config, const TransformOffset& offset, const Color& tint,
                    UnityEngine::Material* templateMaterial);
    void Update(float deltaTime, const UnityEngine::Vector3& tip, const UnityEngine::Vector3& bladeDirection);
    void SetVisible(bool visible);
    void Dispose();

    bool IsInitialized() const { return initialized; }

private:
    struct Joint {
        UnityEngine::GameObject* object = nullptr;
        UnityEngine::Transform* transform = nullptr;
        UnityEngine::Vector3 position{};
        UnityEngine::Vector3 velocity{};
    };

    BoltsConfig config;
    std::vector<Joint> joints;
    UnityEngine::GameObject* root = nullptr;
    UnityEngine::Mesh* mesh = nullptr;
    UnityEngine::Material* material = nullptr;
    bool initialized = false;
    bool primed = false;
};

} // namespace FreeSabers
