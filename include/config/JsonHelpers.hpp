#pragma once

#include "config/SaberConfig.hpp"
#include <rapidjson/document.h>
#include <string>

namespace FreeSabers::Json {

// Safe member lookup. Returns nullptr when the member is missing or the value
// is not an object, so callers never dereference missing fields from presets
// produced by other tools.
const rapidjson::Value* Member(const rapidjson::Value& object, const char* key);

bool IsObject(const rapidjson::Value& value);

float Number(const rapidjson::Value& object, const char* key, float fallback);
float NumberValue(const rapidjson::Value& value, float fallback);
int Int(const rapidjson::Value& object, const char* key, int fallback);
bool Bool(const rapidjson::Value& object, const char* key, bool fallback);
std::string String(const rapidjson::Value& object, const char* key, const std::string& fallback = {});

Color ColorValue(const rapidjson::Value& value, const Color& fallback = Color(1.0f, 1.0f, 1.0f, 1.0f));
Vector3 VectorValue(const rapidjson::Value& value, const Vector3& fallback = Vector3());
Vec2 Vec2Value(const rapidjson::Value& value, const Vec2& fallback = Vec2());

TransformOffset TransformValue(const rapidjson::Value& value);

// Reads both the ReeSabers "controlPoints" spelling and the .freesaber
// "points" spelling. Numeric values are used for Curve, colour values for
// ColorCurve; non-numeric values are treated as 0.
Curve CurveValue(const rapidjson::Value& value);
ColorCurve ColorCurveValue(const rapidjson::Value& value);

} // namespace FreeSabers::Json
