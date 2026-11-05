#pragma once

#include "config/SaberConfig.hpp"
#include <string>
#include <vector>

namespace FreeSabers {

class SaberModel {
public:
    SaberModel() = default;
    ~SaberModel() = default;

    void Initialize(void* saberController, const LayerConfig& layer, bool isLeft);
    void Update(float deltaTime);
    void SetMaterialProperty(const std::string& property, const Vector3& value);
    void SetMaterialProperty(const std::string& property, float value);
    void SetMaterialProperty(const std::string& property, const Color& value);

    void* GetController() const { return controller; }
    void* GetMeshRenderer() const { return meshRenderer; }
    bool IsInitialized() const { return initialized; }

private:
    void* controller = nullptr;
    void* meshRenderer = nullptr;
    void* meshFilter = nullptr;
    void* material = nullptr;
    LayerConfig layerConfig;
    bool isLeft = false;
    bool initialized = false;

    void CreateCustomMesh(const LayerConfig& layer);
    void ApplyMaterial(const LayerConfig& layer);
    void LoadCustomModel(const std::string& path);
};

} // namespace FreeSabers