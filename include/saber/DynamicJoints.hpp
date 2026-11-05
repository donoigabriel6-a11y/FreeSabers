#pragma once

#include "config/SaberConfig.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Mathf.hpp"
#include <vector>

namespace FreeSabers {

class DynamicJoints {
public:
    DynamicJoints() = default;
    ~DynamicJoints() = default;

    void Initialize(void* saberController, const DynamicJointConfig& config);
    void Update(float deltaTime);
    void SetConfig(const DynamicJointConfig& config);

    bool IsInitialized() const { return initialized; }

private:
    void* controller = nullptr;
    DynamicJointConfig config;
    bool initialized = false;

    struct Joint {
        UnityEngine::Vector3 position;
        UnityEngine::Vector3 velocity;
        UnityEngine::Vector3 targetPosition;
        float angle = 0.0f;
        float angularVelocity = 0.0f;
    };

    std::vector<Joint> joints;
    UnityEngine::Vector3 lastPosition;
    UnityEngine::Quaternion lastRotation;

    void CreateJoints();
    void UpdateJointPhysics(float deltaTime);
    void ApplyJointTransforms();
};

} // namespace FreeSabers