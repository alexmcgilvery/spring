/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacySelectMenuServices.h"

//FIXME [SELECT-FLOW-01] These actual backing statements remain disabled until
// the input/display/render binding is reviewed. Source: SelectMenu::Draw.
#if 0
#include "Menu/SelectMenu.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "aGui/Gui.h"

namespace runtime {
// [ display pacing ] Selection has no independent per-frame client update.
void LegacySelectMenuServices::WaitForMenuFrame()
{
	spring_msecs(10).sleep(true);
}

// [ render ] The existing visual companion calls these in this exact order.
void LegacySelectMenuServices::AdvanceDrawCounter()
{
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
}

void LegacySelectMenuServices::ClearMenuScreen()
{
	ClearScreen();
}

void LegacySelectMenuServices::DrawMenuInterface()
{
	//FIXME [SELECT-FLOW-02] Gui::Draw performs deferred destruction in Clean.
	// This render statement therefore also owns lifecycle effects, including
	// destruction of a departed menu. Moving rendering changes when callbacks
	// and GUI objects retire. Keep the actual call visible here; resolve cleanup
	// ownership/headless behavior before moving it out of the serial path.
	agui::gui->Draw();
}
}
#endif
