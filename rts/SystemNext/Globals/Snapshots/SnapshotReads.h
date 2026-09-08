/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "SnapshotTypes.h"
#include "../Graphics/VisualOutput.h"

#include <array>
#include <tuple>
#include <type_traits>

namespace runtime {

template<Stage Source, Slot Selection, bool IsRequired>
struct SnapshotRead {
public:
	static constexpr Stage source = Source;
	static constexpr unsigned slot = static_cast<unsigned>(Selection);
	static constexpr bool required = IsRequired;
};

template<Stage Source, Slot Selection>
using Required = SnapshotRead<Source, Selection, true>;

template<Stage Source, Slot Selection>
using Optional = SnapshotRead<Source, Selection, false>;

template<Stage Source, unsigned Depth, bool IsRequired = false>
using HistoryRead = SnapshotRead<Source, static_cast<Slot>(Depth), IsRequired>;

/** An omitted concern has no inputs and creates no synthetic publication. */
template<class... Reads>
struct SnapshotReads {
public:
	using Entries = std::tuple<Reads...>;

	template<Stage Source, unsigned Depth>
	static consteval std::size_t Index()
	{
		constexpr std::array<bool, size> matches { (Reads::source == Source && Reads::slot == Depth)... };
		for (std::size_t i = 0; i < size; ++i) {
			if (matches[i])
				return i;
		}
		return size;
	}

public:
	static constexpr std::size_t size = sizeof...(Reads);

	template<Stage Source, unsigned Depth>
	static constexpr bool contains = ((Reads::source == Source && Reads::slot == Depth) || ...);

	template<Stage Source>
	static constexpr bool hasSource = ((Reads::source == Source) || ...);

	template<Stage Source, unsigned Depth>
	static constexpr bool isRequired = ((Reads::source == Source && Reads::slot == Depth && Reads::required) || ...);
};

/** Mode-local schemas override only the payloads and concerns that exist. */
struct ModeContracts {
public:
	using InputData = void;
	using SessionData = void;
	using SimulationData = void;
	using DisplayData = void;

	using InputReads = SnapshotReads<>;
	using SessionReads = SnapshotReads<>;
	using DisplayReads = SnapshotReads<>;
	using RenderReads = SnapshotReads<>;
	using PresentReads = SnapshotReads<Required<Stage::Render, Slot::Current>>;
};

template<class Contracts, Stage Source>
using PayloadFor = std::tuple_element_t<static_cast<std::size_t>(Source), std::tuple<
	ApplicationSnapshot, ActivationSnapshot,
	typename Contracts::InputData, typename Contracts::SessionData,
	typename Contracts::SimulationData, typename Contracts::DisplayData,
	RenderedOutput, PresentationReceipt
>>;

template<class Contracts, Stage Consumer>
using ReadsFor = std::tuple_element_t<static_cast<std::size_t>(Consumer), std::tuple<
	SnapshotReads<>, SnapshotReads<>, typename Contracts::InputReads,
	typename Contracts::SessionReads, SnapshotReads<>, typename Contracts::DisplayReads,
	typename Contracts::RenderReads, typename Contracts::PresentReads
>>;

namespace detail {

constexpr bool IsVisual(Stage stage)
{
	return stage >= Stage::Display && stage <= Stage::Present;
}

/**
 * Why: Current may only read an associated producer which can precede this stage.
 * Forward/current and self/current edges are structurally impossible, so required
 * current cycles are rejected without introducing a scheduler. Historical visual
 * feedback is optional on the logical side to permit headless and bootstrap work.
 */
constexpr bool ValidRead(Stage consumer, Stage source, unsigned slot, bool required)
{
	if (source >= Stage::Count)
		return false;

	if (!IsVisual(consumer) && IsVisual(source) && (slot == 0 || required))
		return false;

	if (slot != 0)
		return true;

	return source < consumer;
}

template<class Contracts, Stage Consumer, class Reads>
struct CompiledReads;

template<class Contracts, Stage Consumer, class... Reads>
struct CompiledReads<Contracts, Consumer, SnapshotReads<Reads...>> {
public:
	static_assert(((Reads::source < Stage::Count) && ...), "Unknown snapshot source");
	static_assert(((!std::is_void_v<PayloadFor<Contracts, Reads::source>>) && ...), "Source has no payload declaration");
	static_assert((ValidRead(Consumer, Reads::source, Reads::slot, Reads::required) && ...), "Impossible current dependency or required visual feedback");

	static consteval bool Unique()
	{
		constexpr std::array<Stage, sizeof...(Reads)> sources { Reads::source... };
		constexpr std::array<unsigned, sizeof...(Reads)> slots { Reads::slot... };
		for (std::size_t i = 0; i < sources.size(); ++i) {
			for (std::size_t j = i + 1; j < sources.size(); ++j) {
				if (sources[i] == sources[j] && slots[i] == slots[j])
					return false;
			}
		}
		return true;
	}

	static_assert(Unique(), "Duplicate or conflicting snapshot read");

	static auto Descriptors()
	{
		return std::array<ReadDescriptor, sizeof...(Reads)> {{
			{ Reads::source, Reads::slot, Reads::required, typeid(PayloadFor<Contracts, Reads::source>) }...
		}};
	}
};

} // namespace detail
} // namespace runtime
