#include "saber/SparkSystem.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "main.hpp"
#include "UnityEngine/ParticleSystem.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/Object.hpp"

using namespace UnityEngine;

namespace FreeSabers {

void SparkSystem::Initialize(void* saberController, const SparkConfig& config) {
    this->controller = saberController;
    this->config = config;

    if (!config.enabled) return;

    initialized = true;
    PaperLogger.info("SparkSystem initialized");
}

void SparkSystem::Update(float deltaTime) {
    if (!initialized) return;
}

void SparkSystem::SetConfig(const SparkConfig& config) {
    this->config = config;
}

void SparkSystem::EmitOnCut(int noteType, int cutDirection, float swingRating) {
    if (!initialized) return;
    if (!config.enabled) return;
    PaperLogger.info("SparkSystem: EmitOnCut called");
}

void SparkSystem::CreateParticleSystem() {
    // Disabled for now
}

void SparkSystem::EmitParticles(const UnityEngine::Vector3& position, const UnityEngine::Vector3& direction, float rating) {
    // Disabled for now
}

} // namespace FreeSabers