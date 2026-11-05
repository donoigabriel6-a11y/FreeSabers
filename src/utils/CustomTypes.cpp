#include "utils/CustomTypes.hpp"
#include "saber/SaberManager.hpp"
#include "config/Config.hpp"
#include "main.hpp"

using namespace FreeSabers;
using namespace UnityEngine;

DEFINE_TYPE(FreeSabers, FreeSabersController);
DEFINE_TYPE(FreeSabers, FreeSabersSettingsController);

void FreeSabersController::ctor() {
    saberManager = nullptr;
    initialized = false;
}

void FreeSabersController::Awake() {
    PaperLogger.info("FreeSabersController Awake");
    saberManager = &SaberManager::GetInstance();
    initialized = true;
}

void FreeSabersController::Update() {
    if (!initialized || !saberManager) return;
    saberManager->Update(Time::get_deltaTime());
}

void FreeSabersController::OnDestroy() {
    PaperLogger.info("FreeSabersController OnDestroy");
    initialized = false;
}

void FreeSabersSettingsController::ctor() {
    saberManager = nullptr;
}

void FreeSabersSettingsController::Awake() {
    PaperLogger.info("FreeSabersSettingsController Awake");
    saberManager = &SaberManager::GetInstance();
}

void FreeSabersSettingsController::OnEnable() {
    PaperLogger.info("FreeSabersSettingsController OnEnable");
}

void FreeSabersSettingsController::OnDisable() {
    PaperLogger.info("FreeSabersSettingsController OnDisable");
}