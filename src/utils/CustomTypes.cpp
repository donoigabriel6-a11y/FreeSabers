#include "utils/CustomTypes.hpp"
#include "saber/SaberManager.hpp"
#include "main.hpp"

#include "UnityEngine/Time.hpp"

using namespace FreeSabers;

DEFINE_TYPE(FreeSabers, FreeSabersController);

void FreeSabersController::ctor() {
    initialized = false;
}

void FreeSabersController::Awake() {
    PaperLogger.info("FreeSabersController Awake");
    SaberManager::GetInstance().Initialize();
    initialized = true;
}

void FreeSabersController::Update() {
    SaberManager::GetInstance().Update(UnityEngine::Time::get_deltaTime());
}

void FreeSabersController::OnDestroy() {
    PaperLogger.info("FreeSabersController OnDestroy");
    SaberManager::GetInstance().OnLevelEnd();
    initialized = false;
}
