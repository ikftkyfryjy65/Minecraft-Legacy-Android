#include "stdafx.h"
#include "Linux_UIController.h"

ConsoleUIController ui;

ConsoleUIController::ConsoleUIController() {}
ConsoleUIController::~ConsoleUIController() {}

void ConsoleUIController::CloseUIScenes(int, bool) {}
bool ConsoleUIController::NavigateToScene(int, EUIScene, void*, EUILayer, EUIGroup) { return false; }
bool ConsoleUIController::NavigateBack(int, bool, EUIScene, EUILayer) { return false; }
void ConsoleUIController::CloseAllPlayersScenes() {}
bool ConsoleUIController::GetMenuDisplayed(int) { return false; }

void ConsoleUIController::render() {}
void ConsoleUIController::setTileOrigin(S32, S32) {}
CustomDrawData* ConsoleUIController::setupCustomDraw(UIScene*, IggyCustomDrawCallbackRegion*) { return 0; }
CustomDrawData* ConsoleUIController::calculateCustomDraw(IggyCustomDrawCallbackRegion*) { return 0; }
void ConsoleUIController::endCustomDraw(IggyCustomDrawCallbackRegion*) {}
void ConsoleUIController::beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion*, CustomDrawData*) {}
