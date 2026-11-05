#include "saber/TrailRenderer.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "main.hpp"

namespace FreeSabers {

void TrailRenderer::Initialize(void* saberController, const TrailConfig& config) {
    this->controller = saberController;
    this->config = config;

    initialized = true;
    PaperLogger.info("TrailRenderer initialized");
}

void TrailRenderer::Update(float deltaTime) {
    if (!initialized) return;
}

void TrailRenderer::SetConfig(const TrailConfig& config) {
    this->config = config;
}

} // namespace FreeSabers