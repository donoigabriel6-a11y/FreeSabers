#include "saber/GlowEffect.hpp"
#include "utils/VisualHelpers.hpp"
#include "main.hpp"

#include "UnityEngine/Bounds.hpp"

#include <algorithm>
#include <cmath>

namespace FreeSabers {

bool GlowEffect::Initialize(UnityEngine::Transform* parent, const BladeConfig& blade, const GlowConfig& glow,
                            const TransformOffset& offset, const Color& tint, UnityEngine::Material* templateMaterial) {
    if (!parent || !glow.enabled || !templateMaterial) return false;

    config = glow;
    bladeConfig = blade;
    time = 0.0f;
    cutPulse = 0.0f;

    Color glowColor = config.color;
    if (glowColor.r > 0.98f && glowColor.g > 0.98f && glowColor.b > 0.98f) {
        glowColor = tint;
    }
    baseAlpha = std::clamp(0.16f * config.intensity, 0.05f, 0.55f);

    const float radiusMultiplier = 1.0f + std::clamp(config.radius, 0.0f, 3.0f);
    mesh = Visuals::BuildBladeMesh(bladeConfig, glowColor.WithAlpha(baseAlpha), radiusMultiplier, true);
    if (!mesh) return false;
    mesh->set_bounds(UnityEngine::Bounds(UnityEngine::Vector3{0,0,0}, UnityEngine::Vector3{200,200,200}));

    material = Visuals::CloneMaterial(templateMaterial);
    if (!material) {
        PaperLogger.warn("FreeSabers: failed to clone glow material, trying fallback");
        material = Visuals::CreateFallbackTrailMaterial();
        if (!material) {
            Visuals::SafeDestroy(mesh);
            mesh = nullptr;
            return false;
        }
    }

    Visuals::SetColorIfPresent(material, "_Color", Visuals::ToUnity(glowColor.WithAlpha(baseAlpha)));
    Visuals::SetColorIfPresent(material, "_EmissionColor", Visuals::ToUnity(glowColor));
    Visuals::SetFloatIfPresent(material, "_EmissionIntensity", config.intensity);
    Visuals::SetIntIfPresent(material, "_Cull", 0);
    Visuals::SetIntIfPresent(material, "_ZWrite", 0);
    Visuals::SetIntIfPresent(material, "_SrcBlend", 5);
    Visuals::SetIntIfPresent(material, "_DstBlend", 10);
    Visuals::SetIntIfPresent(material, "_ZTest", 4);
    // Render behind the blade so the shell reads as a glow, not a second blade.
    material->set_renderQueue(3001);

    root = Visuals::CreateMeshObject("FreeSabersGlow", parent, mesh, material);
    if (!root) {
        Visuals::SafeDestroy(material);
        Visuals::SafeDestroy(mesh);
        material = nullptr;
        mesh = nullptr;
        return false;
    }

    Visuals::ApplyTransform(root->get_transform(), offset);
    renderer = root->GetComponent<UnityEngine::MeshRenderer*>();
    if (renderer) renderer->set_sortingOrder(3003);

    if (config.useLight) {
        lightObject = UnityEngine::GameObject::New_ctor(StringW("FreeSabersGlowLight"));
        lightObject->get_transform()->set_parent(parent);
        Visuals::ApplyTransform(lightObject->get_transform(), offset);
        light = lightObject->AddComponent<UnityEngine::Light*>();
        if (light) {
            light->set_color(Visuals::ToUnity(glowColor));
            light->set_intensity(config.intensity);
        }
    }

    initialized = true;
    PaperLogger.info("FreeSabers: glow built (radius x%.2f, intensity %.2f)", radiusMultiplier, config.intensity);
    return true;
}

void GlowEffect::Update(float deltaTime, const Color& tint) {
    if (!initialized) return;
    time += deltaTime;
    cutPulse = std::max(0.0f, cutPulse - deltaTime * 4.0f);

    float pulse = 1.0f;
    if (config.pulseEnabled) {
        pulse += std::sin(time * config.pulseSpeed * 6.28318530718f) * config.pulseAmount * 0.5f;
    }
    pulse += cutPulse * 0.8f;

    Color glowColor = config.color;
    if (glowColor.r > 0.98f && glowColor.g > 0.98f && glowColor.b > 0.98f) {
        glowColor = tint;
    }
    if (!config.pulseGradient.Empty()) {
        glowColor = config.pulseGradient.Evaluate(std::fmod(time * std::max(config.pulseSpeed, 0.01f), 1.0f));
    }

    const float alpha = std::clamp(baseAlpha * pulse, 0.0f, 0.9f);
    Visuals::SetColorIfPresent(material, "_Color", Visuals::ToUnity(glowColor.WithAlpha(alpha)));
    Visuals::SetColorIfPresent(material, "_EmissionColor", Visuals::ToUnity(glowColor));
    if (light) light->set_intensity(std::max(0.0f, config.intensity * pulse));
}

void GlowEffect::PulseOnCut(float strength) {
    cutPulse = std::clamp(strength, 0.0f, 1.0f);
}

void GlowEffect::SetVisible(bool visible) {
    if (Visuals::IsAlive(root)) root->SetActive(visible);
    if (Visuals::IsAlive(lightObject)) lightObject->SetActive(visible && config.useLight);
}

void GlowEffect::Dispose() {
    if (Visuals::IsAlive(root)) UnityEngine::Object::Destroy(root);
    if (Visuals::IsAlive(lightObject)) UnityEngine::Object::Destroy(lightObject);
    root = nullptr;
    lightObject = nullptr;
    renderer = nullptr;
    light = nullptr;
    Visuals::SafeDestroy(material);
    Visuals::SafeDestroy(mesh);
    material = nullptr;
    mesh = nullptr;
    initialized = false;
}

} // namespace FreeSabers
