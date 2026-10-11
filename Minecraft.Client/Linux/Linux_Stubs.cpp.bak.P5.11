/* Linux/Android — remaining linker symbols. */
#include "stdafx.h"
#include "Common/UI/UIController.h"
#include "Common/Network/GameNetworkManager.h"
#include "Xbox/Social/SocialManager.h"

/* ---- Globals ---- */
C_4JProfile ProfileManager;
C4JStorage  StorageManager;
C_4JInput   InputManager;
CGameNetworkManager g_NetworkManager;

/* ---- C4JStorage methods ---- */
C4JStorage::ESaveGameState C4JStorage::DoesSaveExist(bool*) { return (C4JStorage::ESaveGameState)0; }
C4JStorage::PROFILESETTINGS* C4JStorage::GetDashboardProfileSettings(int) { return 0; }
void  C4JStorage::WriteToProfile(int, bool, bool) {}
void  C4JStorage::ForceQueuedProfileWrites(int) {}
void* C4JStorage::GetGameDefinedProfileData(int) { return 0; }

/* ---- C4JInput methods ---- */
void C_4JInput::SetJoypadMapVal(int, unsigned char) {}
void C_4JInput::SetJoypadStickAxisMap(int, unsigned int, unsigned int) {}
void C_4JInput::SetJoypadStickTriggerMap(int, unsigned int, unsigned int) {}

/* ---- CGameNetworkManager ---- */
int  CGameNetworkManager::GetPlayerCount() { return 0; }
bool CGameNetworkManager::IsLeavingGame() { return false; }
bool CGameNetworkManager::IsLocalGame() { return false; }
bool CGameNetworkManager::IsHost() { return false; }

/* ---- UIController ---- */
void UIController::DisplayGamertag(unsigned int, bool) {}
void UIController::RefreshTooltips(unsigned int) {}
void UIController::SetEnableTooltips(unsigned int, BOOL) {}
void UIController::ShowSavingMessage(unsigned int, C4JStorage::ESavingMessage) {}
bool UIController::IsSceneInStack(int, EUIScene) { return false; }
void UIController::SetTooltips(unsigned int, int, int, int, int, int, int, int, int, int, bool) {}
void UIController::HideAllGameUIElements() {}
void UIController::ShowAutosaveCountdownTimer(bool) {}

/* ---- CSocialManager ---- */
CSocialManager* CSocialManager::m_pInstance = 0;
CSocialManager* CSocialManager::Instance() { return 0; }
bool CSocialManager::IsTitleAllowedToPostImages() { return false; }
bool CSocialManager::AreAllUsersAllowedToPostImages() { return false; }

/* ---- Win32 ---- */
void InitializeCriticalSection(_CRITICAL_SECTION*) {}
