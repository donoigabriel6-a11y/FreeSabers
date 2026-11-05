#include "saber/TrailRenderer.hpp"
#include "utils/VisualHelpers.hpp"
#include "main.hpp"

#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace FreeSabers {

bool TrailRenderer::Initialize(const TrailConfig& trailConfig, UnityEngine::Material* templateMaterial) {
    config = trailConfig;
    if (!config.enabled || !templateMaterial) return false;

    maxSamples = std::clamp(static_cast<int>(config.length * 90.0f) + 2, 4, 96);

    vertices = ArrayW<UnityEngine::Vector3>(maxSamples * 2);
    colors = ArrayW<UnityEngine::Color>(maxSamples * 2);
    uvs = ArrayW<UnityEngine::Vector2>(maxSamples * 2);
    triangles = ArrayW<int>((maxSamples - 1) * 6);

    int index = 0;
    for (int i = 0; i < maxSamples - 1; ++i) {
        const int a = i * 2;
        const int b = a + 1;
        const int c = a + 2;
        const int d = a + 3;
        triangles[index++] = a;
        triangles[index++] = b;
        triangles[index++] = d;
        triangles[index++] = a;
        triangles[index++] = d;
        triangles[index++] = c;
    }

    mesh = UnityEngine::Mesh::New_ctor();
    mesh->set_triangles(triangles);

    material = Visuals::CloneMaterial(templateMaterial);
    if (!material) {
        Visuals::SafeDestroy(mesh);
        mesh = nullptr;
        return false;
    }

    Visuals::SetColorIfPresent(material, "_Color", UnityEngine::Color{1.0f, 1.0f, 1.0f, 1.0f});
    Visuals::SetIntIfPresent(material, "_Cull", 0);
    Visuals::SetIntIfPresent(material, "_ZWrite", 0);
    Visuals::SetIntIfPresent(material, "_SrcBlend", 5);   // SrcAlpha
    Visuals::SetIntIfPresent(material, "_DstBlend", config.blendingMode == 1 ? 1 : 10);
    Visuals::SetIntIfPresent(material, "_ZTest", config.alwaysOnTop ? 8 : 4);

    material->set_renderQueue(config.alwaysOnTop ? 4000 : config.renderQueue);
    if (material->HasProperty(StringW("_MainTex"))) {
        material->set_mainTextureScale(Visuals::ToUnity(config.tiling));
        material->set_mainTextureOffset(Visuals::ToUnity(config.uvScroll));
    }

    root = Visuals::CreateMeshObject("FreeSabersTrail_" + config.name, nullptr, mesh, material);
    if (!root) {
        Visuals::SafeDestroy(material);
        Visuals::SafeDestroy(mesh);
        material = nullptr;
        mesh = nullptr;
        return false;
    }

    renderer = root->GetComponent<UnityEngine::MeshRenderer*>();
    if (renderer) {
        renderer->set_sortingOrder(config.alwaysOnTop ? 4000 : config.renderQueue);
        renderer->set_enabled(false);
    }

    initialized = true;
    PaperLogger.info("FreeSabers: trail '%s' built (%d samples, %.3fs)",
                     config.name.c_str(), maxSamples, config.length);
    return true;
}

void TrailRenderer::Update(float deltaTime, const UnityEngine::Vector3& bottom, const UnityEngine::Vector3& tip,
                           float speed, float driverAlpha) {
    if (!initialized || !config.enabled) return;

    elapsed += deltaTime;

    const bool shouldSample = samples.empty() ||
        Visuals::Distance(samples.back().bottom, bottom) > config.minVertexDistance ||
        Visuals::Distance(samples.back().tip, tip) > config.minVertexDistance ||
        (elapsed - lastSampleTime) > (1.0f / 90.0f);

    if (shouldSample) {
        Sample sample;
        sample.bottom = bottom;
        sample.tip = tip;
        sample.time = elapsed;
        sample.speed = speed;
        samples.push_back(sample);
        lastSampleTime = elapsed;
        if (static_cast<int>(samples.size()) > maxSamples) {
            samples.erase(samples.begin());
        }
    }

    while (!samples.empty() && (elapsed - samples.front().time) > config.length) {
        samples.erase(samples.begin());
    }

    if (samples.size() < 2) {
        if (renderer) renderer->set_enabled(false);
        return;
    }

    UnityEngine::Vector3 cameraRight{1.0f, 0.0f, 0.0f};
    if (auto* camera = UnityEngine::Camera::get_main()) {
        if (auto* transform = camera->get_transform()) cameraRight = transform->get_right();
    }

    const float speedBoost = 1.0f + std::max(speed, 0.0f) * config.speedWidth;
    const int used = static_cast<int>(samples.size());
    const UnityEngine::Vector3 trailOffset = Visuals::ToUnity(config.transform.position);

    for (int i = 0; i < used; ++i) {
        const Sample& sample = samples[i];
        const float age = std::clamp((elapsed - sample.time) / std::max(config.length, 0.001f), 0.0f, 1.0f);
        const float widthScale = std::max(config.widthOverAge.Empty() ? 1.0f : config.widthOverAge.Evaluate(age), 0.01f) * speedBoost;

        UnityEngine::Vector3 left;
        UnityEngine::Vector3 right;

        if (config.IsTipTrail()) {
            const float offset = std::clamp(config.offset, 0.0f, 2.0f);
            const UnityEngine::Vector3 point = Visuals::Add(
                Visuals::Lerp(sample.bottom, sample.tip, offset), trailOffset);
            const float halfWidth = config.width * 0.5f * widthScale;
            left = Visuals::Add(point, Visuals::Scale(cameraRight, -halfWidth));
            right = Visuals::Add(point, Visuals::Scale(cameraRight, halfWidth));
        } else {
            const UnityEngine::Vector3 center = Visuals::Scale(Visuals::Add(sample.bottom, sample.tip), 0.5f);
            const UnityEngine::Vector3 half = Visuals::Scale(Visuals::Sub(sample.tip, sample.bottom), 0.5f * widthScale);
            left = Visuals::Add(Visuals::Sub(center, half), trailOffset);
            right = Visuals::Add(Visuals::Add(center, half), trailOffset);
        }

        Color baseColor = config.colorOverAge.Empty()
            ? Color(1.0f, 1.0f, 1.0f, 1.0f)
            : config.colorOverAge.Evaluate(age);
        const float alphaCurve = config.alphaOverAge.Empty() ? 1.0f : config.alphaOverAge.Evaluate(age);
        const float fade = std::pow(1.0f - age, 0.65f);
        const float alpha = std::clamp(baseColor.a * alphaCurve * fade *
                                        (1.0f + std::max(speed, 0.0f) * config.speedAlpha) *
                                        std::clamp(driverAlpha, 0.0f, 2.0f), 0.0f, 1.0f);

        const int index = i * 2;
        vertices[index] = left;
        vertices[index + 1] = right;
        colors[index] = UnityEngine::Color{baseColor.r, baseColor.g, baseColor.b, alpha};
        colors[index + 1] = UnityEngine::Color{baseColor.r, baseColor.g, baseColor.b, alpha};
        uvs[index] = UnityEngine::Vector2{0.0f, 1.0f - age};
        uvs[index + 1] = UnityEngine::Vector2{1.0f, 1.0f - age};
    }

    // Degenerate unused slots so the fixed triangle buffer stays invisible.
    const UnityEngine::Vector3 lastLeft = used > 0 ? vertices[(used - 1) * 2] : UnityEngine::Vector3{0.0f, 0.0f, 0.0f};
    const UnityEngine::Vector3 lastRight = used > 0 ? vertices[(used - 1) * 2 + 1] : UnityEngine::Vector3{0.0f, 0.0f, 0.0f};
    for (int i = used; i < maxSamples; ++i) {
        vertices[i * 2] = lastLeft;
        vertices[i * 2 + 1] = lastRight;
        colors[i * 2] = UnityEngine::Color{0.0f, 0.0f, 0.0f, 0.0f};
        colors[i * 2 + 1] = UnityEngine::Color{0.0f, 0.0f, 0.0f, 0.0f};
        uvs[i * 2] = UnityEngine::Vector2{0.0f, 0.0f};
        uvs[i * 2 + 1] = UnityEngine::Vector2{1.0f, 0.0f};
    }

    mesh->set_vertices(vertices);
    mesh->set_colors(colors);
    mesh->set_uv(uvs);
    if (renderer) renderer->set_enabled(true);
}

void TrailRenderer::SetVisible(bool visible) {
    if (Visuals::IsAlive(root)) root->SetActive(visible);
}

void TrailRenderer::Clear() {
    samples.clear();
    elapsed = 0.0f;
    lastSampleTime = 0.0f;
    if (renderer) renderer->set_enabled(false);
}

void TrailRenderer::Dispose() {
    samples.clear();
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
