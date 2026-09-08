/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../../Modes/ModeIdentity.h"

#include <memory>
#include <typeindex>
#include <utility>

namespace runtime {

/** Owning, type-checked startup data passed between retired and new activations. */
class Handoff {
public:
	Handoff() = default;

	template<class T>
	static Handoff Own(T value)
	{
		return Handoff(std::make_shared<const T>(std::move(value)), typeid(T));
	}

	template<class T>
	const T* Get() const
	{
		return type == typeid(T) ? static_cast<const T*>(data.get()) : nullptr;
	}

private:
	Handoff(std::shared_ptr<const void> data, std::type_index type)
		: data(std::move(data))
		, type(type)
	{
	}

private:
	std::shared_ptr<const void> data;
	std::type_index type {typeid(void)};
};

enum class LifecycleAction {
	SwitchMode,
	Reload,
	Exit,
};

/** Only completed Session work may issue a normal lifecycle request. */
struct LifecycleRequest {
public:
	LifecycleAction action = LifecycleAction::SwitchMode;
	ModeKind target = ModeKind::Inactive;
	Handoff handoff;
};

} // namespace runtime
