#include "saber/DynamicJoints.hpp"
#include "utils/VisualHelpers.hpp"
#include "main.hpp"

#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Transform.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace FreeSabers {

bool DynamicJoints::Initialize(const BoltsConfig& boltsConfig, const TransformOffset& offset, const Color& tint,
                               UnityEngine::Material* templateMaterial) {
    config = boltsConfig;
    primed = false;
    (void)offset;
    if (!config.enabled || !templateMaterial) return false;

    const int jointCount = std::clamp(config.joints, 1, 12);

    Color jointColor = config.color;
    if (jointColor.r > 0.98f && jointColor.g > 0.98f && jointColor.b > 0.98f) {
        jointColor = tint;
    }

    BladeConfig cylinder;
    cylinder.radius = std::max(config.size, 0.002f);
    cylinder.zOffsetFrom = -cylinder.radius * 1.2f;
    cylinder.zOffsetTo = cylinder.radius * 1.2f;
    cylinder.profile.points = {{0.0f, 1.0f}, {1.0f, 1.0f}};
    cylinder.verticalResolution = 3;
    cylinder.horizontalResolution = 6;
    cylinder.startCap = true;
    cylinder.endCap = true;
    cylinder.baseColor = jointColor;

    mesh = Visuals::BuildBladeMesh(cylinder, jointColor, 1.0f, true);
    if (!mesh) return false;

    material = Visuals::CloneMaterial(templateMaterial);
    if (!material) {
        Visuals::SafeDestroy(mesh);
        mesh = nullptr;
        return false;
    }
    Visuals::SetColorIfPresent(material, "_Color", Visuals::ToUnity(jointColor));
    Visuals::SetColorIfPresent(material, "_EmissionColor", Visuals::ToUnity(jointColor));
    Visuals::SetIntIfPresent(material, "_Cull", 2);

    root = Visuals::CreateMeshObject("FreeSabersBolts", nullptr, nullptr, nullptr);
    if (!root) {
        Visuals::SafeDestroy(material);
        Visuals::SafeDestroy(mesh);
        material = nullptr;
        mesh = nullptr;
        return false;
    }
    joints.reserve(jointCount);
    for (int i = 0; i < jointCount; ++i) {
        Joint joint;
        joint.object = Visuals::CreateMeshObject("FreeSabersBolt" + std::to_string(i), root->get_transform(), mesh, material);
        if (!joint.object) continue;
        joint.transform = joint.object->get_transform();
        joints.push_back(joint);
    }

    initialized = !joints.empty();
    if (initialized) {
        PaperLogger.info("FreeSabers: %d dynamic bolts built", static_cast<int>(joints.size()));
    }
    return initialized;
}

void DynamicJoints::Update(float deltaTime, const UnityEngine::Vector3& tip, const UnityEngine::Vector3& bladeDirection) {
    if (!initialized) return;

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    const UnityEngine::Vector3 down{0.0f, -1.0f, 0.0f};
    const UnityEngine::Vector3 gravity{0.0f, config.gravityY, 0.0f};
    const float stiffness = std::max(config.stiffness, 1.0f);
    const float damping = std::exp(-std::max(config.damping, 0.1f) * dt);

    if (!primed) {
        for (size_t i = 0; i < joints.size(); ++i) {
            joints[i].position = Visuals::Add(tip, Visuals::Scale(down, config.spacing * static_cast<float>(i)));
            joints[i].velocity = UnityEngine::Vector3{0.0f, 0.0f, 0.0f};
        }
        primed = true;
    }

    for (size_t i = 0; i < joints.size(); ++i) {
        Joint& joint = joints[i];
        const UnityEngine::Vector3 target = i == 0
            ? tip
            : Visuals::Add(joints[i - 1].position, Visuals::Scale(down, config.spacing));

        const UnityEngine::Vector3 spring = Visuals::Scale(
            Visuals::Sub(target, joint.position), stiffness);
        joint.velocity = Visuals::Add(joint.velocity, Visuals::Scale(spring, dt));
        joint.velocity = Visuals::Add(joint.velocity, Visuals::Scale(gravity, dt));
        joint.velocity = Visuals::Scale(joint.velocity, damping);
        joint.position = Visuals::Add(joint.position, Visuals::Scale(joint.velocity, dt));

        if (i > 0) {
            const UnityEngine::Vector3 previous = joints[i - 1].position;
            const UnityEngine::Vector3 delta = Visuals::Sub(joint.position, previous);
            const float length = Visuals::Magnitude(delta);
            if (length > config.spacing) {
                joint.position = Visuals::Add(previous, Visuals::Scale(delta, config.spacing / length));
            }
        }

        UnityEngine::Vector3 forward = i == 0
            ? Visuals::Normalize(Visuals::Scale(bladeDirection, -1.0f))
            : Visuals::Normalize(Visuals::Sub(joints[i - 1].position, joint.position));
        if (Visuals::Magnitude(forward) < 0.001f) forward = UnityEngine::Vector3{0.0f, 0.0f, 1.0f};

        joint.transform->set_localPosition(joint.position);
        joint.transform->set_localRotation(UnityEngine::Quaternion::LookRotation(forward, UnityEngine::Vector3{0.0f, 0.0f, 1.0f}));
    }
}

void DynamicJoints::SetVisible(bool visible) {
    if (Visuals::IsAlive(root)) root->SetActive(visible);
}

void DynamicJoints::Dispose() {
    if (Visuals::IsAlive(root)) UnityEngine::Object::Destroy(root);
    root = nullptr;
    for (auto& joint : joints) {
        joint.object = nullptr;
        joint.transform = nullptr;
    }
    joints.clear();
    Visuals::SafeDestroy(material);
    Visuals::SafeDestroy(mesh);
    material = nullptr;
    mesh = nullptr;
    initialized = false;
    primed = false;
}

} // namespace FreeSabers
