#pragma once

#include "config/SaberConfig.hpp"
#include <vector>

namespace FreeSabers {

class TrailRenderer {
public:
    TrailRenderer() = default;
    ~TrailRenderer() = default;

    void Initialize(void* saberController, const TrailConfig& config);
    void Update(float deltaTime);
    void SetConfig(const TrailConfig& config);

    void* GetTrailRenderer() const { return trailRenderer; }
    bool IsInitialized() const { return initialized; }

private:
    void* controller = nullptr;
    void* trailRenderer = nullptr;
    TrailConfig config;
    bool initialized = false;
    float timeSinceStart = 0.0f;

    void CreateTrailMesh();
    void UpdateTrailVertices();
};

} // namespace FreeSabers