#pragma once

#include "config/SaberConfig.hpp"

namespace FreeSabers {

class GlowEffect {
public:
    GlowEffect() = default;
    ~GlowEffect() = default;

    void Initialize(void* saberController, const GlowConfig& config);
    void Update(float deltaTime);
    void SetConfig(const GlowConfig& config);
    void PulseOnCut(float swingRating);

    bool IsInitialized() const { return initialized; }

private:
    void* controller = nullptr;
    void* glowObject = nullptr;
    void* lightComponent = nullptr;
    void* bloomMaterial = nullptr;
    GlowConfig config;
    bool initialized = false;
    float pulseTimer = 0.0f;
    float pulseIntensity = 0.0f;
    bool isPulsing = false;

    void CreateGlowObject();
    void CreateBloomMaterial();
};

} // namespace FreeSabers