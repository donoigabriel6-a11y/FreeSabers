#include "utils/CustomTypes.hpp"
#include "saber/SaberManager.hpp"
#include "main.hpp"

using namespace FreeSabers;

DEFINE_TYPE(FreeSabers, FreeSabersController);
DEFINE_TYPE(FreeSabers, FreeSabersSettingsController);

void FreeSabersController::ctor() {
    initialized = false;
}

void FreeSabersController::Awake() {
    PaperLogger.info("FreeSabersController Awake");
    SaberManager::GetInstance().Initialize();
    initialized = true;
}

void FreeSabersController::OnDestroy() {
    PaperLogger.info("FreeSabersController OnDestroy");
    initialized = false;
}

void FreeSabersSettingsController::ctor() {
}

void FreeSabersSettingsController::Awake() {
    PaperLogger.info("FreeSabersSettingsController Awake");
}

void FreeSabersSettingsController::OnEnable() {
    PaperLogger.info("FreeSabersSettingsController OnEnable");
}

void FreeSabersSettingsController::OnDisable() {
    PaperLogger.info("FreeSabersSettingsController OnDisable");
}
