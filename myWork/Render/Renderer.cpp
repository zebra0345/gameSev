// Render/Renderer.cpp
#include "Renderer.h"

void RenderFrame_Primitive(const RenderInput& in)
{
    // 1) ¿ùµå
    if (in.cb.renderWorld)
        in.cb.renderWorld();

    // 2) UI - HP
    if (in.cb.renderPlayerHP)
        in.cb.renderPlayerHP();

    if (in.cb.renderEnemyHPBar)
        in.cb.renderEnemyHPBar();

    // 3) UI - FloatingTexts
    if (in.floatingTexts && in.cb.drawFloatingTexts)
        in.cb.drawFloatingTexts(*in.floatingTexts);

    // 4) Debug overlay
    if (in.debugOverlay && in.cb.drawDebugOverlay)
        in.cb.drawDebugOverlay();

    // 5) Present
    if (in.cb.present)
        in.cb.present();
}
