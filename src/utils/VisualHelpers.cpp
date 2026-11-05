#include "utils/VisualHelpers.hpp"

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "UnityEngine/MeshFilter.hpp"
#include "UnityEngine/Quaternion.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

namespace FreeSabers::Visuals {

namespace {

bool ContainsIgnoreCase(const std::string& haystack, const char* needle) {
    if (haystack.empty()) return false;
    std::string lowerHaystack = haystack;
    std::transform(lowerHaystack.begin(), lowerHaystack.end(), lowerHaystack.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    std::string lowerNeedle(needle);
    std::transform(lowerNeedle.begin(), lowerNeedle.end(), lowerNeedle.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowerHaystack.find(lowerNeedle) != std::string::npos;
}

} // namespace

UnityEngine::Color ToUnity(const Color& color) {
    return UnityEngine::Color{color.r, color.g, color.b, color.a};
}

UnityEngine::Vector3 ToUnity(const Vector3& vector) {
    return UnityEngine::Vector3{vector.x, vector.y, vector.z};
}

UnityEngine::Vector2 ToUnity(const Vec2& vector) {
    return UnityEngine::Vector2{vector.x, vector.y};
}

Color FromUnity(const UnityEngine::Color& color) {
    return Color(color.r, color.g, color.b, color.a);
}

UnityEngine::Vector3 Add(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b) {
    return UnityEngine::Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}

UnityEngine::Vector3 Sub(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b) {
    return UnityEngine::Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}

UnityEngine::Vector3 Scale(const UnityEngine::Vector3& vector, float scale) {
    return UnityEngine::Vector3{vector.x * scale, vector.y * scale, vector.z * scale};
}

UnityEngine::Vector3 Cross(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b) {
    return UnityEngine::Vector3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

float Dot(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Magnitude(const UnityEngine::Vector3& vector) {
    return std::sqrt(Dot(vector, vector));
}

UnityEngine::Vector3 Normalize(const UnityEngine::Vector3& vector) {
    const float magnitude = Magnitude(vector);
    if (magnitude < 1e-5f) return UnityEngine::Vector3{0.0f, 0.0f, 1.0f};
    return Scale(vector, 1.0f / magnitude);
}

float Distance(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b) {
    return Magnitude(Sub(a, b));
}

UnityEngine::Vector3 Lerp(const UnityEngine::Vector3& a, const UnityEngine::Vector3& b, float t) {
    return UnityEngine::Vector3{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t};
}

UnityEngine::Mesh* BuildBladeMesh(const BladeConfig& blade, const Color& tint, float radiusMultiplier, bool solidColor) {
    const int vertical = std::clamp(blade.verticalResolution, 2, 64);
    const int horizontal = std::clamp(blade.horizontalResolution, 3, 32);
    const int rings = vertical + 1;
    const int ringVertices = horizontal + 1;

    const int capVertices = (blade.startCap ? 1 : 0) + (blade.endCap ? 1 : 0);
    const int vertexCount = rings * ringVertices + capVertices;

    std::vector<UnityEngine::Vector3> vertices;
    std::vector<UnityEngine::Vector3> normals;
    std::vector<UnityEngine::Vector2> uvs;
    std::vector<UnityEngine::Color> colors;
    std::vector<int> indices;

    vertices.reserve(vertexCount);
    normals.reserve(vertexCount);
    uvs.reserve(vertexCount);
    colors.reserve(vertexCount);
    indices.reserve(vertical * horizontal * 6 + horizontal * 6);

    for (int i = 0; i < rings; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(vertical);
        const float z = blade.zOffsetFrom + (blade.zOffsetTo - blade.zOffsetFrom) * t;
        const float profile = blade.profile.Empty() ? 1.0f : std::max(blade.profile.Evaluate(t), 0.001f);
        const float radius = blade.radius * radiusMultiplier * profile * blade.SampleScale(t);

        Color ringColor;
        if (solidColor) {
            ringColor = tint;
        } else {
            ringColor = blade.SampleColor(t);
            ringColor.a = blade.SampleAlpha(t);
        }

        for (int j = 0; j <= horizontal; ++j) {
            const float angle = (static_cast<float>(j) / static_cast<float>(horizontal)) * 6.28318530718f;
            const float cos = std::cos(angle);
            const float sin = std::sin(angle);

            vertices.emplace_back(cos * radius, sin * radius, z);
            normals.emplace_back(cos, sin, 0.0f);
            uvs.emplace_back(static_cast<float>(j) / static_cast<float>(horizontal), t);
            colors.push_back(UnityEngine::Color{ringColor.r, ringColor.g, ringColor.b, ringColor.a});
        }
    }

    for (int i = 0; i < vertical; ++i) {
        for (int j = 0; j < horizontal; ++j) {
            const int a = i * ringVertices + j;
            const int b = (i + 1) * ringVertices + j;
            const int c = b + 1;
            const int d = a + 1;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    if (blade.startCap) {
        const int center = static_cast<int>(vertices.size());
        const float z = blade.zOffsetFrom;
        Color capColor = solidColor ? tint : blade.SampleColor(0.0f);
        capColor.a = solidColor ? tint.a : blade.SampleAlpha(0.0f);
        vertices.emplace_back(0.0f, 0.0f, z);
        normals.emplace_back(0.0f, 0.0f, -1.0f);
        uvs.emplace_back(0.5f, 0.0f);
        colors.push_back(UnityEngine::Color{capColor.r, capColor.g, capColor.b, capColor.a});
        for (int j = 0; j < horizontal; ++j) {
            indices.push_back(center);
            indices.push_back(j);
            indices.push_back(j + 1);
        }
    }

    if (blade.endCap) {
        const int center = static_cast<int>(vertices.size());
        const int ringStart = vertical * ringVertices;
        const float z = blade.zOffsetTo;
        Color capColor = solidColor ? tint : blade.SampleColor(1.0f);
        capColor.a = solidColor ? tint.a : blade.SampleAlpha(1.0f);
        vertices.emplace_back(0.0f, 0.0f, z);
        normals.emplace_back(0.0f, 0.0f, 1.0f);
        uvs.emplace_back(0.5f, 1.0f);
        colors.push_back(UnityEngine::Color{capColor.r, capColor.g, capColor.b, capColor.a});
        for (int j = 0; j < horizontal; ++j) {
            indices.push_back(center);
            indices.push_back(ringStart + j + 1);
            indices.push_back(ringStart + j);
        }
    }

    auto* mesh = UnityEngine::Mesh::New_ctor();

    ArrayW<UnityEngine::Vector3> vertexArray(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) vertexArray[i] = vertices[i];
    mesh->set_vertices(vertexArray);

    ArrayW<UnityEngine::Vector3> normalArray(normals.size());
    for (size_t i = 0; i < normals.size(); ++i) normalArray[i] = normals[i];
    mesh->set_normals(normalArray);

    ArrayW<UnityEngine::Vector2> uvArray(uvs.size());
    for (size_t i = 0; i < uvs.size(); ++i) uvArray[i] = uvs[i];
    mesh->set_uv(uvArray);

    ArrayW<UnityEngine::Color> colorArray(colors.size());
    for (size_t i = 0; i < colors.size(); ++i) colorArray[i] = colors[i];
    mesh->set_colors(colorArray);

    ArrayW<int> indexArray(indices.size());
    for (size_t i = 0; i < indices.size(); ++i) indexArray[i] = indices[i];
    mesh->set_triangles(indexArray);

    return mesh;
}

UnityEngine::GameObject* CreateMeshObject(const std::string& name, UnityEngine::Transform* parent,
                                          UnityEngine::Mesh* mesh, UnityEngine::Material* material) {
    auto* object = UnityEngine::GameObject::New_ctor(StringW(name));
    UnityEngine::Transform* transform = object->get_transform();
    if (parent) transform->set_parent(parent);
    transform->set_localPosition(UnityEngine::Vector3{0.0f, 0.0f, 0.0f});
    transform->set_localRotation(UnityEngine::Quaternion::get_identity());
    transform->set_localScale(UnityEngine::Vector3{1.0f, 1.0f, 1.0f});

    if (mesh) {
        UnityEngine::MeshFilter* filter = object->AddComponent<UnityEngine::MeshFilter*>();
        filter->set_mesh(mesh);
    }
    if (material) {
        UnityEngine::MeshRenderer* renderer = object->AddComponent<UnityEngine::MeshRenderer*>();
        renderer->set_sharedMaterial(material);
    }
    return object;
}

UnityEngine::Material* CloneMaterial(UnityEngine::Material* source) {
    if (!source) return nullptr;
    return UnityEngine::Material::New_ctor(source);
}

UnityEngine::Material* FindSaberMaterial(UnityEngine::Transform* parent, bool trailMaterial,
                                         UnityEngine::MeshRenderer** outRenderer) {
    if (outRenderer) *outRenderer = nullptr;
    if (!parent) return nullptr;

    UnityEngine::Material* fallback = nullptr;
    UnityEngine::MeshRenderer* fallbackRenderer = nullptr;

    auto renderers = parent->GetComponentsInChildren<UnityEngine::MeshRenderer*>(true);
    for (UnityEngine::MeshRenderer* renderer : renderers) {
        if (!renderer) continue;
        UnityEngine::Material* material = renderer->get_sharedMaterial();
        if (!material) material = renderer->get_material();
        if (!material) continue;

        if (!fallback) {
            fallback = material;
            fallbackRenderer = renderer;
        }

        const std::string materialName = ObjectName(material);
        const std::string shaderName = material->get_shader() ? ObjectName(material->get_shader()) : std::string();

        const bool match = trailMaterial
            ? (ContainsIgnoreCase(materialName, "trail") || ContainsIgnoreCase(shaderName, "trail"))
            : (ContainsIgnoreCase(materialName, "saber") || ContainsIgnoreCase(materialName, "blade") ||
               ContainsIgnoreCase(shaderName, "saber") || ContainsIgnoreCase(shaderName, "blade"));

        if (match) {
            if (outRenderer) *outRenderer = renderer;
            return material;
        }
    }

    if (outRenderer) *outRenderer = fallbackRenderer;
    return fallback;
}

void SetColorIfPresent(UnityEngine::Material* material, const char* property, const UnityEngine::Color& color) {
    if (!material) return;
    const StringW name(property);
    if (material->HasProperty(name)) material->SetColor(name, color);
}

void SetFloatIfPresent(UnityEngine::Material* material, const char* property, float value) {
    if (!material) return;
    const StringW name(property);
    if (material->HasProperty(name)) material->SetFloat(name, value);
}

void SetIntIfPresent(UnityEngine::Material* material, const char* property, int value) {
    if (!material) return;
    const StringW name(property);
    if (material->HasProperty(name)) material->SetInt(name, value);
}

bool IsAlive(UnityEngine::Object* object) {
    if (!object) return false;
    // m_CachedPtr is the native pointer; it is nulled when Unity destroys the object.
    return object->__cordl_internal_get_m_CachedPtr().m_value.convert() != nullptr;
}

void SafeDestroy(UnityEngine::Object* object) {
    if (IsAlive(object)) UnityEngine::Object::Destroy(object);
}

void ApplyTransform(UnityEngine::Transform* transform, const TransformOffset& offset) {
    if (!transform) return;
    transform->set_localPosition(ToUnity(offset.position));
    transform->set_localRotation(UnityEngine::Quaternion::Euler(ToUnity(offset.rotation)));
    transform->set_localScale(ToUnity(offset.scale));
}

std::string ObjectName(UnityEngine::Object* object) {
    if (!object) return {};
    const StringW name = object->get_name();
    if (!name) return {};
    return static_cast<std::string>(name);
}

} // namespace FreeSabers::Visuals
