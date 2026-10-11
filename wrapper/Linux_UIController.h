#pragma once
#include "../Common/UI/UIController.h"
/* Linux/Android: ConsoleUIController adds the 6 platform-specific pure
 * virtuals left abstract by the common UIController. Phase 5 = real impl. */
class ConsoleUIController : public UIController
{
public:
    ConsoleUIController();
    virtual ~ConsoleUIController();

    virtual void CloseUIScenes(int iPad, bool forceIPad = false);
    virtual bool NavigateToScene(int iPad, EUIScene scene, void *initData = NULL, EUILayer layer = eUILayer_Scene, EUIGroup group = eUIGroup_PAD);
    virtual bool NavigateBack(int iPad, bool forceUsePad = false, EUIScene eScene = eUIScene_COUNT, EUILayer eLayer = eUILayer_COUNT);
    virtual void CloseAllPlayersScenes();
    virtual bool GetMenuDisplayed(int iPad);

    virtual void render();
    virtual void setTileOrigin(S32 xPos, S32 yPos);
    virtual CustomDrawData *setupCustomDraw(UIScene *scene, IggyCustomDrawCallbackRegion *region);
    virtual CustomDrawData *calculateCustomDraw(IggyCustomDrawCallbackRegion *region);
    virtual void endCustomDraw(IggyCustomDrawCallbackRegion *region);
    virtual void beginIggyCustomDraw4J(IggyCustomDrawCallbackRegion *region, CustomDrawData *customDrawRegion);
};
extern ConsoleUIController ui;
