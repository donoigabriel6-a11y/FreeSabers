#include "saber/SaberModel.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "main.hpp"

namespace FreeSabers {

void SaberModel::Initialize(void* saberController, const LayerConfig& layer, bool isLeft) {
    this->controller = saberController;
    this->layerConfig = layer;
    this->isLeft = isLeft;

    initialized = true;
    PaperLogger.info("SaberModel initialized for %s saber", isLeft ? "Left" : "Right");
}

void SaberModel::Update(float deltaTime) {
    if (!initialized) return;
}

void SaberModel::SetMaterialProperty(const std::string& property, const Vector3& value) {
    // Disabled for now
}

void SaberModel::SetMaterialProperty(const std::string& property, float value) {
    // Disabled for now
}

void SaberModel::SetMaterialProperty(const std::string& property, const Color& value) {
    // Disabled for now
}

void SaberModel::ApplyMaterial(const LayerConfig& layer) {
    // Disabled for now
}

void SaberModel::LoadCustomModel(const std::string& path) {
    PaperLogger.info("Custom model loading not yet implemented: %s", path.c_str());
}

} // namespace FreeSabers