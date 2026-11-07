#include "saber/SaberModel.hpp"
#include "utils/VisualHelpers.hpp"
#include "main.hpp"

#include "UnityEngine/Bounds.hpp"

namespace FreeSabers {

bool SaberModel::Initialize(UnityEngine::Transform* parent, const BladeConfig& blade, const TransformOffset& offset,
                            const Color& tint, UnityEngine::Material* templateMaterial) {
    if (!parent || !blade.enabled || !templateMaterial) return false;

    config = blade;
    mesh = Visuals::BuildBladeMesh(config, tint, 1.0f, false);
    if (!mesh) return false;
    mesh->set_bounds(UnityEngine::Bounds(UnityEngine::Vector3{0,0,0}, UnityEngine::Vector3{200,200,200}));

    material = Visuals::CloneMaterial(templateMaterial);
    if (!material) {
        PaperLogger.warn("FreeSabers: failed to clone blade material, trying fallback");
        material = Visuals::CreateFallbackTrailMaterial();
        if (!material) {
            Visuals::SafeDestroy(mesh);
            mesh = nullptr;
            return false;
        }
    }

    ApplyMaterial(tint);
    material->set_renderQueue(config.renderQueue);
    Visuals::SetIntIfPresent(material, "_Cull", config.cullMode);
    Visuals::SetIntIfPresent(material, "_ZWrite", config.depthWrite ? 1 : 0);

    root = Visuals::CreateMeshObject("FreeSabersBlade", parent, mesh, material);
    if (!root) {
        Visuals::SafeDestroy(material);
        Visuals::SafeDestroy(mesh);
        material = nullptr;
        mesh = nullptr;
        return false;
    }

    Visuals::ApplyTransform(root->get_transform(), offset);
    renderer = root->GetComponent<UnityEngine::MeshRenderer*>();
    if (renderer) renderer->set_sortingOrder(config.renderQueue);

    initialized = true;
    time = 0.0f;
    PaperLogger.info("FreeSabers: blade built (radius %.4f, %d profile keys)",
                     config.radius, static_cast<int>(config.profile.points.size()));
    return true;
}

void SaberModel::ApplyMaterial(const Color& tint) {
    if (!material) return;

    // The material tint is the fallback for shaders that ignore vertex colours.
    Color displayColor = config.colorOverValue.Empty() ? config.baseColor : config.SampleColor(0.5f);
    Color emissionColor = config.emissionColor;

    if (config.colorOverride.force) {
        displayColor = tint;
        emissionColor = tint;
    }

    Visuals::SetColorIfPresent(material, "_Color", Visuals::ToUnity(displayColor));
    Visuals::SetColorIfPresent(material, "_BaseColor", Visuals::ToUnity(displayColor));
    Visuals::SetColorIfPresent(material, "_EmissionColor", Visuals::ToUnity(emissionColor.MultiplyRGB(config.emissionIntensity)));
    Visuals::SetFloatIfPresent(material, "_EmissionIntensity", config.emissionIntensity);
    Visuals::SetFloatIfPresent(material, "_GlowIntensity", config.emissionIntensity * config.glowMultiplier);
    Visuals::SetFloatIfPresent(material, "_FresnelPower", config.fresnelPower);
    Visuals::SetFloatIfPresent(material, "_Fresnel", config.fresnelPower);
    Visuals::SetFloatIfPresent(material, "_Metallic", config.metallic);
    Visuals::SetFloatIfPresent(material, "_Glossiness", 1.0f - config.roughness);
    Visuals::SetFloatIfPresent(material, "_Roughness", config.roughness);
    Visuals::SetFloatIfPresent(material, "_EnvLightIntensity", config.envLightIntensity);
    Visuals::SetFloatIfPresent(material, "_ReflectionIntensity", config.reflectionIntensity);
    Visuals::SetFloatIfPresent(material, "_NormalMapIntensity", config.normalMapIntensity);
    Visuals::SetFloatIfPresent(material, "_BlurFrames", config.blurFrames);
    Visuals::SetFloatIfPresent(material, "_BlurSensitivity", config.blurSensitivity);
    Visuals::SetFloatIfPresent(material, "_GlowMultiplier", config.glowMultiplier);
    Visuals::SetColorIfPresent(material, "_ReflectionColor", Visuals::ToUnity(config.reflectionColor));
    Visuals::SetColorIfPresent(material, "_EnvLightColor", Visuals::ToUnity(config.envLightColor));
    Visuals::SetColorIfPresent(material, "_HandleColor", Visuals::ToUnity(config.handleColor));
    Visuals::SetFloatIfPresent(material, "_HandleRoughness", config.handleRoughness);
}

void SaberModel::Update(float deltaTime, const Color& tint) {
    if (!initialized || !material) return;
    time += deltaTime;

    if (config.colorOverride.force) {
        const Color animated = config.colorOverride.Evaluate(time);
        Visuals::SetColorIfPresent(material, "_Color", Visuals::ToUnity(animated));
        Visuals::SetColorIfPresent(material, "_EmissionColor",
                                   Visuals::ToUnity(animated.MultiplyRGB(config.emissionIntensity)));
    } else {
        // Keep the material in sync if the caller supplied a different tint.
        (void)tint;
    }

    if (config.useTexture && material->HasProperty(StringW("_MainTex"))) {
        material->set_mainTextureScale(Visuals::ToUnity(config.tiling));
        material->set_mainTextureOffset(Visuals::ToUnity(config.uvScroll));
    }
}

UnityEngine::Vector3 SaberModel::GetPoint(float z) const {
    if (!root) return UnityEngine::Vector3{0.0f, 0.0f, z};
    return root->get_transform()->TransformPoint(UnityEngine::Vector3{0.0f, 0.0f, z});
}

void SaberModel::SetVisible(bool visible) {
    if (Visuals::IsAlive(root)) root->SetActive(visible);
}

void SaberModel::Dispose() {
    if (Visuals::IsAlive(root)) UnityEngine::Object::Destroy(root);
    root = nullptr;
    renderer = nullptr;
    Visuals::SafeDestroy(material);
    Visuals::SafeDestroy(mesh);
    material = nullptr;
    mesh = nullptr;
    initialized = false;
}

} // namespace FreeSabers
