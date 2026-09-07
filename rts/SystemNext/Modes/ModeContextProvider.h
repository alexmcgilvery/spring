/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
#include "ModeContexts.h"
namespace runtime {
/**
 * Application-side construction of fresh concern contexts. Concrete providers
 * resolve legacy dependencies; modes receive only their appropriate alternative.
 * No provider implementation or private legacy access is supplied here.
 * Construction must not dispatch callbacks, retire modes, or advance state.
 *
 * Invocation metadata is borrowed only during the immediate concern call. Store
 * copies of identity/time values if frame preparation needs them afterward.
 * Frame outputs belong to this iteration and activation. Render must receive the
 * frame produced for that activation; never reuse it after replacement. EndIteration
 * releases prepared data under the graphics scope, including exception unwinding.
 */
class ModeContextProvider {
public:
	virtual ~ModeContextProvider();
	virtual void BeginIteration(std::uint64_t iteration) = 0;
	virtual void EndIteration() noexcept = 0;
	virtual ModeInputContext Input(const InvocationContext& invocation) = 0;
	virtual ModeSessionContext Session(const InvocationContext& invocation) = 0;
	virtual ModeDisplayContext Display(const InvocationContext& invocation) = 0;
	virtual ModeRenderContext Render(const InvocationContext& invocation) = 0;
};
}
