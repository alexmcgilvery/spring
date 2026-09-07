/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>
namespace runtime {
struct PublishedSimFrame;
/**
 * Owning handle contract for completed state; payload/schema and producer remain
 * unimplemented. The eventual producer must publish immutable owned contents,
 * without mutable aliases or embedded borrows into legacy world objects.
 *
 * Unlike an invocation context, a valid lease may survive the call and teardown.
 * shared_ptr<const T> alone does not guarantee deep immutability or visibility
 * projection. A null lease means no publication is available, never an empty world.
 */
using PublishedFrameLease = std::shared_ptr<const PublishedSimFrame>;
}
