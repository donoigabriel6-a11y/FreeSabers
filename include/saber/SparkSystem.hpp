#pragma once

#include "config/SaberConfig.hpp"
#include "UnityEngine/Vector3.hpp"
#include <vector>

namespace FreeSabers {

class SparkSystem {
public:
    SparkSystem() = default;
    ~SparkSystem() = default;

    void Initialize(void* saberController, const SparkConfig& config);
    void Update(float deltaTime);
    void SetConfig(const SparkConfig& config);
    void EmitOnCut(int noteType, int cutDirection, float swingRating);

    bool IsInitialized() const { return initialized; }

private:
    void* controller = nullptr;
    void* particleSystem = nullptr;
    SparkConfig config;
    bool initialized = false;
    std::vector<void*> particlePool; // Pool of particle systems

    void CreateParticleSystem();
    void EmitParticles(const UnityEngine::Vector3& position, const UnityEngine::Vector3& direction, float rating);
};

} // namespace FreeSabers