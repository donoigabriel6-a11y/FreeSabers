#include "saber/GlowEffect.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "main.hpp"

namespace FreeSabers {

void GlowEffect::Initialize(void* saberController, const GlowConfig& config) {
    this->controller = saberController;
    this->config = config;

    if (!config.enabled) return;

    initialized = true;
    PaperLogger.info("GlowEffect initialized");
}

void GlowEffect::Update(float deltaTime) {
    if (!initialized) return;
}

void GlowEffect::SetConfig(const GlowConfig& config) {
    this->config = config;
}

void GlowEffect::PulseOnCut(float swingRating) {
    if (!initialized) return;
}

void GlowEffect::CreateGlowObject() {
    // Disabled for now
}

void GlowEffect::CreateBloomMaterial() {
    // Disabled for now
}

} // namespace FreeSabers