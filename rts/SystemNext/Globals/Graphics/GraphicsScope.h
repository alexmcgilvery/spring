/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once
namespace runtime {
/**
 * Lifetime of actual graphics synchronization supplied by a host implementation.
 * The host acquires before returning this scope and releases in its destructor.
 * No GL/context acquisition is implemented by this abstract contract.
 */
class GraphicsScope {
public:
	virtual ~GraphicsScope();
};
}
