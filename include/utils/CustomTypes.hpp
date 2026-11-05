#pragma once

#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/register.hpp"
#include "UnityEngine/MonoBehaviour.hpp"

// Custom MonoBehaviours used by FreeSabers.
DECLARE_CLASS_CODEGEN(FreeSabers, FreeSabersController, UnityEngine::MonoBehaviour)
{
    DECLARE_INSTANCE_FIELD(bool, initialized);

    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, OnDestroy);

    DECLARE_CTOR(ctor);
};

DECLARE_CLASS_CODEGEN(FreeSabers, FreeSabersSettingsController, UnityEngine::MonoBehaviour)
{
    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, OnEnable);
    DECLARE_INSTANCE_METHOD(void, OnDisable);

    DECLARE_CTOR(ctor);
};

namespace FreeSabers {
    inline void RegisterCustomTypes() {
        custom_types::Register::AutoRegister();
    }
}
