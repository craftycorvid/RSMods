#pragma once

#include <string_view>

#include "GamePhase.hpp"

struct GameLoopState; // ModManager.hpp — transitional; dissolves as mods own their own state.

namespace Framework {
	struct ModContext;

	// Internal interface for built-in mods.
	class IMod {
	public:
		virtual ~IMod() = default;

		virtual std::string_view Id() const = 0;               // Stable, UNIQUE; matches the mod's INI key prefix.

		// Must be cheap and side-effect free.
		virtual bool IsEnabled(const ModContext&) const { return true; }

		virtual void OnInitialize(ModContext&)      {} // Host services are ready.
		virtual void OnShutdown(ModContext&)        {}

		virtual void OnSettingsChanged(ModContext&) {} // Applied AND delivered on MainThread.

		// OnEnabled MUST be strongly exception-safe: if it throws it becomes Faulted and
		// OnDisabled is NOT called, so it must leave no partially-applied game state behind.
		virtual void OnEnabled(ModContext&)  {} // Acquire game state here.
		virtual void OnDisabled(ModContext&) {} // REVERT game state here.

		virtual void OnTick(ModContext&)      {} // Every active tick, any phase (incl. Loading -> guard unsafe memory).
		virtual void OnMenuTick(ModContext&)  {} // In menus.
		virtual void OnSongEnter(ModContext&) {} // Edge: this mod became active-in-a-song (incl. enabled mid-song).
		virtual void OnSongTick(ModContext&)  {} // In a song.
		virtual void OnSongExit(ModContext&)  {} // Edge: this mod stopped being active-in-a-song (incl. disabled mid-song).
	};
}
