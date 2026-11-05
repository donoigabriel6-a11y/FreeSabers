#include "config/JsonHelpers.hpp"

#include <algorithm>
#include <cctype>

namespace FreeSabers::Json {

namespace {

bool HasKey(const rapidjson::Value& object, const char* key) {
    return object.IsObject() && object.HasMember(key) && !object[key].IsNull();
}

const rapidjson::Value* PointsArray(const rapidjson::Value& value) {
    if (!value.IsObject()) return nullptr;
    if (const auto* points = Member(value, "controlPoints")) {
        if (points->IsArray()) return points;
    }
    if (const auto* points = Member(value, "points")) {
        if (points->IsArray()) return points;
    }
    return nullptr;
}

} // namespace

const rapidjson::Value* Member(const rapidjson::Value& object, const char* key) {
    if (!object.IsObject()) return nullptr;
    auto it = object.FindMember(key);
    if (it == object.MemberEnd() || it->value.IsNull()) return nullptr;
    return &it->value;
}

bool IsObject(const rapidjson::Value& value) {
    return value.IsObject();
}

float NumberValue(const rapidjson::Value& value, float fallback) {
    if (value.IsNumber()) return value.GetFloat();
    if (value.IsBool()) return value.GetBool() ? 1.0f : 0.0f;
    return fallback;
}

float Number(const rapidjson::Value& object, const char* key, float fallback) {
    const auto* value = Member(object, key);
    return value ? NumberValue(*value, fallback) : fallback;
}

int Int(const rapidjson::Value& object, const char* key, int fallback) {
    const auto* value = Member(object, key);
    if (!value) return fallback;
    if (value->IsInt()) return value->GetInt();
    if (value->IsNumber()) return static_cast<int>(value->GetDouble());
    if (value->IsBool()) return value->GetBool() ? 1 : 0;
    return fallback;
}

bool Bool(const rapidjson::Value& object, const char* key, bool fallback) {
    const auto* value = Member(object, key);
    if (!value) return fallback;
    if (value->IsBool()) return value->GetBool();
    if (value->IsNumber()) return value->GetDouble() != 0.0;
    return fallback;
}

std::string String(const rapidjson::Value& object, const char* key, const std::string& fallback) {
    const auto* value = Member(object, key);
    if (!value) return fallback;
    if (value->IsString()) return std::string(value->GetString(), value->GetStringLength());
    return fallback;
}

Color ColorValue(const rapidjson::Value& value, const Color& fallback) {
    if (value.IsArray() && value.Size() >= 3) {
        Color result;
        result.r = NumberValue(value[0], fallback.r);
        result.g = NumberValue(value[1], fallback.g);
        result.b = NumberValue(value[2], fallback.b);
        result.a = value.Size() >= 4 ? NumberValue(value[3], fallback.a) : 1.0f;
        return result;
    }
    if (value.IsObject()) {
        const bool hasAny = HasKey(value, "r") || HasKey(value, "g") || HasKey(value, "b") || HasKey(value, "a");
        if (!hasAny) return fallback;
        Color result = fallback;
        result.r = Number(value, "r", fallback.r);
        result.g = Number(value, "g", fallback.g);
        result.b = Number(value, "b", fallback.b);
        result.a = Number(value, "a", fallback.a);
        return result;
    }
    return fallback;
}

Vector3 VectorValue(const rapidjson::Value& value, const Vector3& fallback) {
    if (value.IsArray() && value.Size() >= 3) {
        return Vector3(
            NumberValue(value[0], fallback.x),
            NumberValue(value[1], fallback.y),
            NumberValue(value[2], fallback.z));
    }
    if (value.IsObject()) {
        return Vector3(
            Number(value, "x", fallback.x),
            Number(value, "y", fallback.y),
            Number(value, "z", fallback.z));
    }
    return fallback;
}

Vec2 Vec2Value(const rapidjson::Value& value, const Vec2& fallback) {
    if (value.IsArray() && value.Size() >= 2) {
        return Vec2(NumberValue(value[0], fallback.x), NumberValue(value[1], fallback.y));
    }
    if (value.IsObject()) {
        return Vec2(Number(value, "x", fallback.x), Number(value, "y", fallback.y));
    }
    return fallback;
}

TransformOffset TransformValue(const rapidjson::Value& value) {
    TransformOffset result;
    if (!value.IsObject()) return result;
    if (const auto* position = Member(value, "Position")) {
        result.position = VectorValue(*position, result.position);
    } else if (const auto* position = Member(value, "position")) {
        result.position = VectorValue(*position, result.position);
    }
    if (const auto* rotation = Member(value, "Rotation")) {
        result.rotation = VectorValue(*rotation, result.rotation);
    } else if (const auto* rotation = Member(value, "rotation")) {
        result.rotation = VectorValue(*rotation, result.rotation);
    }
    if (const auto* scale = Member(value, "Scale")) {
        result.scale = VectorValue(*scale, result.scale);
    } else if (const auto* scale = Member(value, "scale")) {
        result.scale = VectorValue(*scale, result.scale);
    }
    return result;
}

Curve CurveValue(const rapidjson::Value& value) {
    Curve curve;
    if (!value.IsObject()) return curve;

    curve.interpolationType = Int(value, "interpolationType", 1);
    curve.valueFrom = Number(value, "valueFrom", 0.0f);
    curve.valueTo = Number(value, "valueTo", 1.0f);

    if (const auto* points = PointsArray(value)) {
        curve.points.reserve(points->Size());
        for (const auto& point : points->GetArray()) {
            if (!point.IsObject()) continue;
            CurvePoint key;
            key.time = Number(point, "time", 0.0f);
            if (const auto* rawValue = Member(point, "value")) {
                key.value = NumberValue(*rawValue, 0.0f);
            }
            curve.points.push_back(key);
        }
    }

    std::sort(curve.points.begin(), curve.points.end(), [](const CurvePoint& a, const CurvePoint& b) {
        return a.time < b.time;
    });
    return curve;
}

ColorCurve ColorCurveValue(const rapidjson::Value& value) {
    ColorCurve curve;
    if (!value.IsObject()) return curve;

    curve.interpolationType = Int(value, "interpolationType", 1);
    curve.valueFrom = Number(value, "valueFrom", 0.0f);
    curve.valueTo = Number(value, "valueTo", 1.0f);

    if (const auto* points = PointsArray(value)) {
        curve.points.reserve(points->Size());
        for (const auto& point : points->GetArray()) {
            if (!point.IsObject()) continue;
            GradientPoint key;
            key.time = Number(point, "time", 0.0f);
            if (const auto* rawValue = Member(point, "value")) {
                key.value = ColorValue(*rawValue, Color(1.0f, 1.0f, 1.0f, 1.0f));
            }
            curve.points.push_back(key);
        }
    }

    std::sort(curve.points.begin(), curve.points.end(), [](const GradientPoint& a, const GradientPoint& b) {
        return a.time < b.time;
    });
    return curve;
}

} // namespace FreeSabers::Json
