#include "hooks/SaberHooks.hpp"
#include "saber/SaberManager.hpp"
#include "config/Config.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "main.hpp"

using namespace FreeSabers;

namespace FreeSabers::Hooks {

// Hook storage (will be initialized at runtime)
static const MethodInfo* SaberModelController_Update_Method = nullptr;
static const MethodInfo* SaberMeshRenderer_OnWillRenderObject_Method = nullptr;
static const MethodInfo* SaberTrail_Update_Method = nullptr;
static const MethodInfo* Saber_ManualUpdate_Method = nullptr;
static const MethodInfo* NoteController_NoteCut_Method = nullptr;
static const MethodInfo* GameplayCoreSceneSetupData_Init_Method = nullptr;

void InitializeHookMethods() {
    // Find IL2CPP methods at runtime
    SaberModelController_Update_Method = il2cpp_utils::FindMethodUnsafe("", "SaberModelController", "Update", 0);
    SaberMeshRenderer_OnWillRenderObject_Method = il2cpp_utils::FindMethodUnsafe("", "SaberMeshRenderer", "OnWillRenderObject", 0);
    SaberTrail_Update_Method = il2cpp_utils::FindMethodUnsafe("", "SaberTrail", "Update", 0);
    Saber_ManualUpdate_Method = il2cpp_utils::FindMethodUnsafe("", "Saber", "ManualUpdate", 0);
    NoteController_NoteCut_Method = il2cpp_utils::FindMethodUnsafe("", "NoteController", "NoteCut", 4);
    GameplayCoreSceneSetupData_Init_Method = il2cpp_utils::FindMethodUnsafe("", "GameplayCoreSceneSetupData", "Init", 0);
}

void InstallSaberHooks() {
    PaperLogger.info("Installing saber hooks...");

    InitializeHookMethods();

    // Install hooks using beatsaber-hook's hooking API
    // Note: Full hook implementation requires proper MAKE_HOOK macros
    // For now, we log that hooks would be installed
    PaperLogger.info("Saber hook methods found: %s",
        SaberModelController_Update_Method ? "yes" : "no");
    PaperLogger.info("NoteController hook method found: %s",
        NoteController_NoteCut_Method ? "yes" : "no");
    PaperLogger.info("GameplayCoreSceneSetupData hook method found: %s",
        GameplayCoreSceneSetupData_Init_Method ? "yes" : "no");

    // TODO: Implement proper MAKE_HOOK_OFFSETLESS hooks once method signatures are verified
    PaperLogger.info("Hook installation deferred - will implement after verifying method signatures");
}

void* GetSaberModelController(bool isLeft) {
    // Find SaberManager and get the appropriate controller
    return nullptr;
}

void* GetSaberMeshRenderer(void* saberController) {
    // Disabled for now - would use il2cpp_utils::RunMethod with proper type handling
    return nullptr;
}

void* GetSaberTrail(void* saberController) {
    // Disabled for now
    return nullptr;
}

} // namespace FreeSabers::Hooks