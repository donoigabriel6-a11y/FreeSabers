#include "saber/SparkSystem.hpp"
#include "utils/VisualHelpers.hpp"
#include "main.hpp"

#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace FreeSabers {

namespace {
constexpr float kDegToRad = 0.017453292519943295f;
}

bool SparkSystem::Initialize(const SparkConfig& sparkConfig, UnityEngine::Material* templateMaterial, const Color& tint) {
    config = sparkConfig;
    tintColor = tint;
    if (!config.enabled || !templateMaterial) return false;

    capacity = std::clamp(config.count, 8, 160);
    particles.resize(capacity);

    vertices = ArrayW<UnityEngine::Vector3>(capacity * 4);
    colors = ArrayW<UnityEngine::Color>(capacity * 4);
    uvs = ArrayW<UnityEngine::Vector2>(capacity * 4);
    triangles = ArrayW<int>(capacity * 6);

    for (int i = 0; i < capacity; ++i) {
        const int base = i * 4;
        const int index = i * 6;
        triangles[index] = base;
        triangles[index + 1] = base + 1;
        triangles[index + 2] = base + 2;
        triangles[index + 3] = base;
        triangles[index + 4] = base + 2;
        triangles[index + 5] = base + 3;
    }

    for (int i = 0; i < capacity * 4; ++i) {
        vertices[i] = UnityEngine::Vector3{0.0f, 0.0f, 0.0f};
        colors[i] = UnityEngine::Color{0.0f, 0.0f, 0.0f, 0.0f};
        uvs[i] = UnityEngine::Vector2{0.0f, 0.0f};
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
    Visuals::SetIntIfPresent(material, "_SrcBlend", 5);
    Visuals::SetIntIfPresent(material, "_DstBlend", 1);
    material->set_renderQueue(3010);

    root = Visuals::CreateMeshObject("FreeSabersSparks", nullptr, mesh, material);
    if (!root) {
        Visuals::SafeDestroy(material);
        Visuals::SafeDestroy(mesh);
        material = nullptr;
        mesh = nullptr;
        return false;
    }

    renderer = root->GetComponent<UnityEngine::MeshRenderer*>();
    if (renderer) {
        renderer->set_sortingOrder(3010);
        renderer->set_enabled(false);
    }

    initialized = true;
    PaperLogger.info("FreeSabers: sparks built (%d particles)", capacity);
    return true;
}

float SparkSystem::Random01() {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return static_cast<float>(rngState & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

SparkSystem::Particle* SparkSystem::AcquireParticle() {
    for (auto& particle : particles) {
        if (!particle.alive) return &particle;
    }
    // Recycle the oldest particle when the budget is full.
    Particle* oldest = &particles[0];
    float bestAge = -1.0f;
    for (auto& particle : particles) {
        const float ageFraction = particle.lifetime > 0.0f ? particle.age / particle.lifetime : 1.0f;
        if (ageFraction > bestAge) {
            bestAge = ageFraction;
            oldest = &particle;
        }
    }
    return oldest;
}

void SparkSystem::Emit(const UnityEngine::Vector3& position, const UnityEngine::Vector3& direction, float strength) {
    if (!initialized || !config.enabled || capacity <= 0) return;

    const int burst = std::clamp(static_cast<int>(capacity * 0.09f *
                                                 std::clamp(strength, 0.2f, 1.6f)), 3, 28);

    const UnityEngine::Vector3 baseDirection = Visuals::Normalize(direction);

    for (int i = 0; i < burst; ++i) {
        Particle* particle = AcquireParticle();
        if (!particle) return;

        const UnityEngine::Vector3 randomDirection = Visuals::Normalize(UnityEngine::Vector3{
            Random01() * 2.0f - 1.0f,
            Random01() * 2.0f - 1.0f,
            Random01() * 2.0f - 1.0f});

        const float spread = std::tan(config.spread * kDegToRad * 0.5f);
        const UnityEngine::Vector3 direction2 = Visuals::Normalize(Visuals::Add(
            baseDirection, Visuals::Scale(randomDirection, spread * config.burstSpread)));

        particle->alive = true;
        particle->age = 0.0f;
        particle->lifetime = std::max(0.02f, config.minLifetime +
                                              (config.maxLifetime - config.minLifetime) * Random01());
        particle->seed = Random01() * 100.0f;
        particle->position = Visuals::Add(position, Visuals::Scale(randomDirection, config.size * 0.5f * Random01()));
        particle->velocity = Visuals::Scale(direction2, config.velocity * (0.55f + Random01() * 0.9f));
    }
}

void SparkSystem::Update(float deltaTime, const UnityEngine::Vector3& tip, const Color& tint, float speed) {
    if (!initialized || !config.enabled) return;

    elapsed += deltaTime;
    const float dt = std::min(deltaTime, 0.05f);

    if (config.emissionMode == 2 && speed > 1.2f) {
        emitTimer += dt;
        const float period = std::max(config.triggerPeriod, 0.02f);
        while (emitTimer >= period) {
            emitTimer -= period;
            Emit(tip, UnityEngine::Vector3{0.0f, 0.0f, 1.0f}, std::min(1.0f, speed * 0.25f));
        }
    }

    bool anyAlive = false;
    for (auto& particle : particles) {
        if (!particle.alive) continue;

        particle.age += dt;
        if (particle.age >= particle.lifetime) {
            particle.alive = false;
            continue;
        }

        anyAlive = true;
        particle.velocity.y += config.gravityY * dt;
        const float damping = std::exp(-config.damping * dt);
        particle.velocity = Visuals::Scale(particle.velocity, damping);

        if (config.turbulencePower > 0.0f) {
            const float phase = particle.seed + elapsed * config.turbulenceScale;
            particle.velocity = Visuals::Add(particle.velocity, UnityEngine::Vector3{
                std::sin(phase) * config.turbulencePower * dt,
                std::cos(phase * 1.3f) * config.turbulencePower * dt,
                std::sin(phase * 0.7f) * config.turbulencePower * dt});
        }

        particle.position = Visuals::Add(particle.position, Visuals::Scale(particle.velocity, dt));
    }

    if (!anyAlive) {
        if (renderer) renderer->set_enabled(false);
        return;
    }

    UnityEngine::Vector3 cameraRight{1.0f, 0.0f, 0.0f};
    UnityEngine::Vector3 cameraUp{0.0f, 1.0f, 0.0f};
    if (auto* camera = UnityEngine::Camera::get_main()) {
        if (auto* transform = camera->get_transform()) {
            cameraRight = transform->get_right();
            cameraUp = transform->get_up();
        }
    }

    tintColor = tint;
    WriteBuffers(cameraRight, cameraUp);
    if (renderer) renderer->set_enabled(true);
}

void SparkSystem::WriteBuffers(const UnityEngine::Vector3& cameraRight, const UnityEngine::Vector3& cameraUp) {
    for (int i = 0; i < capacity; ++i) {
        const Particle& particle = particles[i];
        const int base = i * 4;

        if (!particle.alive) {
            for (int j = 0; j < 4; ++j) {
                vertices[base + j] = UnityEngine::Vector3{0.0f, 0.0f, 0.0f};
                colors[base + j] = UnityEngine::Color{0.0f, 0.0f, 0.0f, 0.0f};
                uvs[base + j] = UnityEngine::Vector2{0.0f, 0.0f};
            }
            continue;
        }

        const float lifeT = std::clamp(particle.age / std::max(particle.lifetime, 0.001f), 0.0f, 1.0f);
        Color color = config.colorOverLifetime.Empty() ? config.color : config.colorOverLifetime.Evaluate(lifeT);
        const float alphaCurve = config.alphaOverLifetime.Empty() ? 1.0f : config.alphaOverLifetime.Evaluate(lifeT);
        const float scaleCurve = config.scaleOverLifetime.Empty() ? 1.0f : config.scaleOverLifetime.Evaluate(lifeT);

        if (color.r > 0.98f && color.g > 0.98f && color.b > 0.98f) {
            color.r = tintColor.r;
            color.g = tintColor.g;
            color.b = tintColor.b;
        }

        const float alpha = std::clamp(color.a * alphaCurve * (1.0f - lifeT * 0.35f), 0.0f, 1.0f);
        const float halfSize = std::max(0.001f, config.size * scaleCurve * (1.0f - lifeT * 0.5f));

        const UnityEngine::Vector3 right = Visuals::Scale(cameraRight, halfSize);
        const UnityEngine::Vector3 up = Visuals::Scale(cameraUp, halfSize);

        vertices[base] = Visuals::Sub(Visuals::Sub(particle.position, right), up);
        vertices[base + 1] = Visuals::Sub(Visuals::Add(particle.position, right), up);
        vertices[base + 2] = Visuals::Add(Visuals::Add(particle.position, right), up);
        vertices[base + 3] = Visuals::Add(Visuals::Sub(particle.position, right), up);

        const UnityEngine::Color vertexColor{color.r, color.g, color.b, alpha};
        colors[base] = vertexColor;
        colors[base + 1] = vertexColor;
        colors[base + 2] = vertexColor;
        colors[base + 3] = vertexColor;

        uvs[base] = UnityEngine::Vector2{0.0f, 0.0f};
        uvs[base + 1] = UnityEngine::Vector2{1.0f, 0.0f};
        uvs[base + 2] = UnityEngine::Vector2{1.0f, 1.0f};
        uvs[base + 3] = UnityEngine::Vector2{0.0f, 1.0f};
    }

    mesh->set_vertices(vertices);
    mesh->set_colors(colors);
    mesh->set_uv(uvs);
}

void SparkSystem::SetVisible(bool visible) {
    if (Visuals::IsAlive(root)) root->SetActive(visible);
}

void SparkSystem::Dispose() {
    particles.clear();
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
