/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "ModeContexts.h"
namespace runtime {
enum class DisplayPhase { Absent, BeforeGraphics, WithGraphics };
/**
 * Common concern contract for every mode. Capabilities describe scheduling;
 * they are independent of controller inheritance or legacy Update/Draw methods.
 * Context alternatives keep each concrete mode's dependencies explicit. Calling
 * an absent concern or supplying another mode's context is a programming error.
 * Concern bodies are still outlines; interface conformance is not implementation.
 */
class IMode {
public:
	virtual ~IMode();
	const ModeKind kind;
	const bool handlesSession;
	const DisplayPhase displayPhase;
	virtual void Input(const ModeInputContext& context) = 0;
	virtual void Session(const ModeSessionContext& context);
	virtual void Display(const ModeDisplayContext& context);
	virtual void Render(const ModeRenderContext& context) = 0;
protected:
	IMode(ModeKind kind, bool handlesSession, DisplayPhase displayPhase);
};
}
