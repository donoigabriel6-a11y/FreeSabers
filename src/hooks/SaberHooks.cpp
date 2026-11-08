#include "hooks/SaberHooks.hpp"
#include "saber/SaberManager.hpp"
#include "main.hpp"

#include "GlobalNamespace/SaberModelController.hpp"
#include "GlobalNamespace/Saber.hpp"
#include "GlobalNamespace/SaberType.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/ColorManager.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/ComboController.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Transform.hpp"

namespace FreeSabers::Hooks {

MAKE_HOOK_MATCH(SaberModelController_Init, &GlobalNamespace::SaberModelController::Init, void,
                GlobalNamespace::SaberModelController* self, UnityEngine::Transform* parent,
                GlobalNamespace::Saber* saber, UnityEngine::Color tint) {
    SaberModelController_Init(self, parent, saber, tint);
    SaberManager::GetInstance().OnSaberModelInit(self, parent, saber, tint);
}

MAKE_HOOK_MATCH(ScoreController_HandleNoteWasCut, &GlobalNamespace::ScoreController::HandleNoteWasCut, void,
                GlobalNamespace::ScoreController* self, GlobalNamespace::NoteController* noteController,
                ByRef<GlobalNamespace::NoteCutInfo> noteCutInfo) {
    ScoreController_HandleNoteWasCut(self, noteController, noteCutInfo);
    SaberManager::GetInstance().OnNoteCut(noteCutInfo->saberType, *noteCutInfo);
}

MAKE_HOOK_MATCH(ScoreController_HandleNoteWasMissed, &GlobalNamespace::ScoreController::HandleNoteWasMissed, void,
                GlobalNamespace::ScoreController* self, GlobalNamespace::NoteController* noteController) {
    ScoreController_HandleNoteWasMissed(self, noteController);
    SaberManager::GetInstance().OnNoteMissed();
}

MAKE_HOOK_MATCH(GameplayCoreInstaller_InstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void,
                GlobalNamespace::GameplayCoreInstaller* self) {
    // Clear the previous level before the scene creates its saber models.
    SaberManager::GetInstance().OnLevelStart();
    GameplayCoreInstaller_InstallBindings(self);
}

MAKE_HOOK_MATCH(AudioTimeSyncController_OnDestroy, &GlobalNamespace::AudioTimeSyncController::OnDestroy, void,
                GlobalNamespace::AudioTimeSyncController* self) {
    SaberManager::GetInstance().OnLevelEnd();
    AudioTimeSyncController_OnDestroy(self);
}

MAKE_HOOK_MATCH(ColorManager_ColorForSaberType, &GlobalNamespace::ColorManager::ColorForSaberType,
                UnityEngine::Color, GlobalNamespace::ColorManager* self, GlobalNamespace::SaberType saberType) {
    if (auto color = SaberManager::GetInstance().GetGameplayColor(saberType)) return *color;
    return ColorManager_ColorForSaberType(self, saberType);
}

MAKE_HOOK_MATCH(ColorManager_EffectsColorForSaberType, &GlobalNamespace::ColorManager::EffectsColorForSaberType,
                UnityEngine::Color, GlobalNamespace::ColorManager* self, GlobalNamespace::SaberType saberType) {
    if (auto color = SaberManager::GetInstance().GetGameplayColor(saberType)) return *color;
    return ColorManager_EffectsColorForSaberType(self, saberType);
}

void InstallSaberHooks() {
    PaperLogger.info("FreeSabers: installing hooks");

    INSTALL_HOOK(PaperLogger, SaberModelController_Init);
    INSTALL_HOOK(PaperLogger, ScoreController_HandleNoteWasCut);
    INSTALL_HOOK(PaperLogger, ScoreController_HandleNoteWasMissed);
    INSTALL_HOOK(PaperLogger, GameplayCoreInstaller_InstallBindings);
    INSTALL_HOOK(PaperLogger, AudioTimeSyncController_OnDestroy);
    INSTALL_HOOK(PaperLogger, ColorManager_ColorForSaberType);
    INSTALL_HOOK(PaperLogger, ColorManager_EffectsColorForSaberType);

    PaperLogger.info("FreeSabers: hooks installed");
}

} // namespace FreeSabers::Hooks