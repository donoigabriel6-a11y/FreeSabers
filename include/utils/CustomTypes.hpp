#pragma once

#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/register.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "saber/SaberManager.hpp"

// Custom type registration for FreeSabers
// This allows creating custom C# types from C++

// Declare and define FreeSabersController class
DECLARE_CLASS_CODEGEN(FreeSabers, FreeSabersController, UnityEngine::MonoBehaviour)
{
    DECLARE_INSTANCE_FIELD(SaberManager*, saberManager);
    DECLARE_INSTANCE_FIELD(bool, initialized);

    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, Update);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_CTOR(ctor);
};

// Declare and define FreeSabersSettingsController class
DECLARE_CLASS_CODEGEN(FreeSabers, FreeSabersSettingsController, UnityEngine::MonoBehaviour)
{
    DECLARE_INSTANCE_FIELD(SaberManager*, saberManager);

    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, OnEnable);
    DECLARE_INSTANCE_METHOD(void, OnDisable);

    DECLARE_CTOR(ctor);
};

// Register all custom types
namespace FreeSabers {
    void RegisterCustomTypes() {
        custom_types::Register::AutoRegister();
    }
}