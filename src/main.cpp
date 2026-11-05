#include "main.hpp"
#include "config/Config.hpp"
#include "saber/SaberManager.hpp"
#include "hooks/SaberHooks.hpp"
#include "hooks/UIHooks.hpp"
#include "utils/CustomTypes.hpp"
#include "scotland2/shared/modloader.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"

using namespace UnityEngine;

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

MOD_EXTERN_FUNC void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();

    Paper::Logger::RegisterFileContextId(PaperLogger.tag);

    PaperLogger.info("FreeSabers setup started!");
    PaperLogger.info("Mod ID: %s, Version: %s", MOD_ID, VERSION);
}

MOD_EXTERN_FUNC void late_load() noexcept {
    il2cpp_functions::Init();

    PaperLogger.info("Initializing FreeSabers...");

    // Initialize config system
    // Get the mod data path from the modloader
    std::string modDataPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/FreeSabers";
    Config::GetInstance().Initialize(modDataPath);

    // Register custom types
    RegisterCustomTypes();

    // Install saber hooks
    Hooks::InstallSaberHooks();

    // Install UI hooks
    UI::InstallUIHooks();

    // Create FreeSabersController GameObject to persist across scenes
    Il2CppString* nameStr = il2cpp_utils::createcsstr("FreeSabersController");
    auto* go = GameObject::New_ctor(nameStr);
    Object::DontDestroyOnLoad(go);
    go->AddComponent<FreeSabers::FreeSabersController*>();

    PaperLogger.info("FreeSabers initialized successfully!");
}

MOD_EXTERN_FUNC void on_game_scene_loaded(int scene) noexcept {
    // Called when a game scene is loaded
    PaperLogger.info("Game scene loaded: %d", scene);

    // Reinitialize saber manager for new scene
    SaberManager::GetInstance().ReloadPresets();
}

MOD_EXTERN_FUNC void on_game_scene_unloaded(int scene) noexcept {
    // Called when a game scene is unloaded
    PaperLogger.info("Game scene unloaded: %d", scene);
}