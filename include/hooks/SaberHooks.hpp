#pragma once

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

namespace FreeSabers::Hooks {

// Initialize all saber-related hooks
void InstallSaberHooks();

// Hook functions
void SaberModelController_Update(void* self);
void SaberMeshRenderer_OnWillRenderObject(void* self);
void SaberTrail_Update(void* self);
void Saber_ManualUpdate(void* self);
void NoteController_NoteCut(void* self, void* noteData, void* saber, int cutDirection, float swingRating);
void GameplayCoreSceneSetupData_Init(void* self);

// Helper functions
void* GetSaberModelController(bool isLeft);
void* GetSaberMeshRenderer(void* saberController);
void* GetSaberTrail(void* saberController);

} // namespace FreeSabers::Hooks