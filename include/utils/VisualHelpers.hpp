#pragma once

#include "config/SaberConfig.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Mesh.hpp"
#include "UnityEngine/MeshRenderer.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"

#include <string>

namespace FreeSabers::Visuals {

UnityEngine::Color ToUnity(const Color& color);
UnityEngine::Vector3 ToUnity(const Vector3& vector);
UnityEngine::Vector2 ToUnity(const Vec2& vector);
Color FromUnity(const UnityEngine::Color& color);

UnityEngine::Vector3 Add(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b);
UnityEngine::Vector3 Sub(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b);
UnityEngine::Vector3 Scale(const UnityEngine::Vector3& vector, float scale);
UnityEngine::Vector3 Cross(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b);
float Dot(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b);
float Magnitude(const UnityEngine::Vector3& vector);
UnityEngine::Vector3 Normalize(const UnityEngine::Vector3& vector);
float Distance(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b);
UnityEngine::Vector3 Lerp(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b, float t);

// Builds a closed cylinder along +Z with an optional heat-shrink profile and
// per-vertex colours sampled from the blade mappings.
UnityEngine::Mesh* BuildBladeMesh(const BladeConfig& blade, const Color& tint, float radiusMultiplier, bool solidColor);

UnityEngine::GameObject* CreateMeshObject(const std::string& name, UnityEngine::Transform* parent,
                                          UnityEngine::Mesh* mesh, UnityEngine::Material* material);

UnityEngine::Material* CloneMaterial(UnityEngine::Material* source);

// Finds a material belonging to the vanilla saber. When trailMaterial is true
// the trail shader is preferred (it is transparent and uses vertex colours).
UnityEngine::Material* FindSaberMaterial(UnityEngine::Transform* parent, bool trailMaterial,
                                         UnityEngine::MeshRenderer** outRenderer = nullptr);

// Finds or creates a suitable material for trail rendering, with bladeMaterial as fallback.
UnityEngine::Material* FindTrailMaterial(UnityEngine::Transform* parent, UnityEngine::Material* bladeMaterial);

// Creates a simple unlit transparent material for trails (fallback when no suitable material found).
UnityEngine::Material* CreateFallbackTrailMaterial();

void SetColorIfPresent(UnityEngine::Material* material, const char* property, const UnityEngine::Color& color);
void SetFloatIfPresent(UnityEngine::Material* material, const char* property, float value);
void SetIntIfPresent(UnityEngine::Material* material, const char* property, int value);

bool IsAlive(UnityEngine::Object* object);
void SafeDestroy(UnityEngine::Object* object);

void ApplyTransform(UnityEngine::Transform* transform, const TransformOffset& offset);

std::string ObjectName(UnityEngine::Object* object);

} // namespace FreeSabers::Visuals
