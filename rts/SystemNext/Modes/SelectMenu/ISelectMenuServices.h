/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace runtime {
/**
 * Define the reusable selection-menu visual operations before binding them.
 *
 * These are abstract extraction requirements, not an implemented legacy
 * adapter. Implementations must use the single backing implementation of each
 * operation and must not invoke SelectMenu::Draw. The service object outlives
 * the GUI tree and retains no SelectMenu pointer across cleanup or callbacks.
 * All calls run synchronously on the current graphics/context-owning path.
 */
class ISelectMenuServices {
public:
	virtual ~ISelectMenuServices();

	/** Preserve the existing spring_msecs(10).sleep(true) idle pacing. */
	virtual void WaitForMenuFrame() = 0;

	/** Preserve max(1U, drawFrame + 1), including unsigned wrap behavior. */
	virtual void AdvanceDrawCounter() = 0;

	/**
	 * Reuse ClearScreen: color/depth clear, projection/modelview reset, blend
	 * setup, texture enable and draw color. This is more than a buffer clear.
	 */
	virtual void ClearMenuScreen() = 0;

	/**
	 * Reuse agui::Gui::Draw, including cleanup before GUI graphics setup and
	 * background-to-foreground traversal. The outer runtime owns swap and its
	 * loading guard; this operation must neither acquire another guard nor swap.
	 * Deferred cleanup may retire the SelectMenu backing object. Do not access
	 * that object after this call, including from cleanup or timing epilogues.
	 */
	virtual void DrawMenuInterface() = 0;

	//FIXME [SELECT-MENU-02] Gui::RmElement searches only installed elements;
	// it ignores toBeAdded. Gui::Clean first installs additions and then deletes
	// removals. A newly created/closed window can therefore escape removal.
	// Define and test activation/departure before first cleanup, duplicate removal,
	// and destructor-enqueued child-window removals before providing a binding.
	// Preserve current behavior or approve an explicit separate GUI lifetime fix;
	// do not introduce an eager delete in a callback (Gui::RmElement forbids it).

	//FIXME [SELECT-MENU-03] Gui::Draw is empty under HEADLESS; CPreGame's
	// destructor and SpringApp::Kill also omit GUI cleanup in that variant.
	// A fake DrawMenuInterface must not imply graphics/headless retirement parity.
	// Establish whether headless selection is supported and exercise its lifetime
	// separately before activation; keep variant-specific backing behavior intact.
};
}
