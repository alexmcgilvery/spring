/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SnapshotReads.h"

#include <cassert>

namespace runtime {

// Forward declarations: only the manager may construct an acquired view.
class SnapshotManager;

/**
 * A source window retains its immutable selections independently of the parent
 * view. Accessors exist only for declared slots. Reading never consumes actions.
 */
template<class Payload, Stage Source, class Reads>
class SnapshotWindow {
public:
	decltype(auto) Current() const requires Reads::template contains<Source, 0>
	{
		return History<0>();
	}

	decltype(auto) Previous() const requires Reads::template contains<Source, 1>
	{
		return History<1>();
	}

	decltype(auto) Older() const requires Reads::template contains<Source, 2>
	{
		return History<2>();
	}

	template<unsigned Depth>
	decltype(auto) History() const requires Reads::template contains<Source, Depth>
	{
		const auto& lease = selections->publications[Reads::template Index<Source, Depth>()];
		const auto* value = lease ? static_cast<const Payload*>(lease->data.get()) : nullptr;

		if constexpr (Reads::template isRequired<Source, Depth>) {
			assert(value != nullptr);
			return *value;
		} else {
			return value;
		}
	}

	template<unsigned Depth>
	const PublicationIdentity* Identity() const requires Reads::template contains<Source, Depth>
	{
		const auto& lease = selections->publications[Reads::template Index<Source, Depth>()];
		return lease ? &lease->identity : nullptr;
	}

private:
	template<class, Stage> friend class SnapshotView;

	explicit SnapshotWindow(std::shared_ptr<const detail::ResolvedSnapshots> selections)
		: selections(std::move(selections))
	{
	}

private:
	std::shared_ptr<const detail::ResolvedSnapshots> selections;
};

/** Frozen, owning invocation inputs. A mode cannot request undeclared live state. */
template<class Contracts, Stage Consumer>
class SnapshotView {
public:
	using Reads = ReadsFor<Contracts, Consumer>;
	using Windows = std::tuple<
		SnapshotWindow<PayloadFor<Contracts, Stage::Application>, Stage::Application, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Activation>, Stage::Activation, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Input>, Stage::Input, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Session>, Stage::Session, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Simulation>, Stage::Simulation, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Display>, Stage::Display, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Render>, Stage::Render, Reads>,
		SnapshotWindow<PayloadFor<Contracts, Stage::Present>, Stage::Present, Reads>
	>;

	const InvocationContext& Invocation() const
	{
		return selections->invocation;
	}

	const auto& Application() const requires Reads::template hasSource<Stage::Application>
	{
		return Window<Stage::Application>();
	}

	const auto& Activation() const requires Reads::template hasSource<Stage::Activation>
	{
		return Window<Stage::Activation>();
	}

	const auto& Input() const requires Reads::template hasSource<Stage::Input>
	{
		return Window<Stage::Input>();
	}

	const auto& Session() const requires Reads::template hasSource<Stage::Session>
	{
		return Window<Stage::Session>();
	}

	const auto& Simulation() const requires Reads::template hasSource<Stage::Simulation>
	{
		return Window<Stage::Simulation>();
	}

	const auto& Display() const requires Reads::template hasSource<Stage::Display>
	{
		return Window<Stage::Display>();
	}

	const auto& Render() const requires Reads::template hasSource<Stage::Render>
	{
		return Window<Stage::Render>();
	}

	const auto& Present() const requires Reads::template hasSource<Stage::Present>
	{
		return Window<Stage::Present>();
	}

private:
	friend class SnapshotManager;

	explicit SnapshotView(detail::ResolvedSnapshots selections)
		: selections(std::make_shared<const detail::ResolvedSnapshots>(std::move(selections)))
		, windows(MakeWindows(this->selections, std::make_index_sequence<8> {}))
	{
	}

	template<Stage Source>
	const auto& Window() const
	{
		return std::get<static_cast<std::size_t>(Source)>(windows);
	}

	template<std::size_t... Indices>
	static Windows MakeWindows(const std::shared_ptr<const detail::ResolvedSnapshots>& selections, std::index_sequence<Indices...>)
	{
		return Windows { SnapshotWindow<PayloadFor<Contracts, static_cast<Stage>(Indices)>, static_cast<Stage>(Indices), Reads>(selections)... };
	}

private:
	std::shared_ptr<const detail::ResolvedSnapshots> selections;
	Windows windows;
};

template<class Mode, Stage Consumer>
using SnapshotsFor = SnapshotView<typename Mode::Contracts, Consumer>;

} // namespace runtime
