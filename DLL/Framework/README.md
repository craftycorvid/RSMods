# Mod Framework (core)

Generalizes the `CCEffect` pattern to *all* mods so that a mod owns its own state, activation, and lifecycle 
instead of `ModManager` doing it, and adding a mod is adding one `.cpp` rather than editing the orchestrator.

> **Scope: lifecycle + conflict arbitration.** The render-callback subsystem (D3D `EndScene`
> hooks + cross-thread quiesce) is still a **follow-up**; teardown here is synchronous.

## Design boundaries (read first)

- **Two layers, kept separate.** `IMod`/`ModContext` are an **internal C++ API** for built-in
  mods. They are **not** the third-party plugin boundary and must not be used as an ABI. 
- **Activation ownership lives in the framework, not in each mod.** A mod says *what* it wants
  (enabled? render callback?); the registry decides *whether* and *when* it runs.
- **No input/WndProc surface.** `Keybindings` already owns input, and a consumable message hook
  risks swallowing host-critical messages (`WM_CLOSE`, `WM_COPYDATA`).

## Host wiring (`dllmain.cpp`)

- `MainThread` - `InstantiatePending()` → `ApplyStartupMods` → `DispatchInitialize()`, then
  `Registry().Tick(phase, loopState)` each loop, and `Registry().Shutdown()` after the loop.
- `WndProc` `WM_COPYDATA` - `Keybindings::UpdateSettingsOnGUIChange`, which *queues* the
  settings mutation onto the registry instead of applying it on the message thread.

## Adding a mod

1. `Mods/MyMod.hpp`: a class deriving `Framework::IMod`; read settings via the context, keep
   state as members.
2. `Mods/MyMod.cpp`:
   ```cpp
   #include "stdafx.h"
   #include "MyMod.hpp"
   static Framework::ModRegistrar<MyMod> _myReg;   // MUST be in the .cpp, never a header.
   ```
   The static registrar only pushes a POD factory node at load time (loader-lock safe); the
   registry constructs the mod later from MainThread (`InstantiatePending`).
3. If you migrated logic out of `ModManager`, delete it there and drop any `GameLoopState` field.

`Id()` must be **unique**; duplicates are rejected at registration.

## Lifecycle state machine

```
Registered ──OnInitialize──▶ Inactive ──OnEnabled──▶ Active
     │                          │  ▲                    │
     │ OnInitialize throws     │  └────OnDisabled──────┘
     ▼                          │                       │
  Faulted ◀─────────────────────┴───────────────────────┘
              OnEnabled or tick hook throws
```

- **Inactive** means initialized but not effectively active. It covers a mod that never activated,
  one the user disabled, and one **suppressed** by losing a resource conflict; their next valid
  transition is identical (the suppressed-vs-disabled distinction survives only in the log line).
- **Effective activation** = `IsEnabled()` **and** winning every resource it contends for. Only
  `Active` mods get tick hooks.
- **Ordering guarantees** (edges are per-mod, not global phase edges):
  - Activate in a song: `OnEnabled → OnSongEnter → OnTick → OnSongTick`
  - Deactivate in a song: `OnSongExit → OnDisabled`
  - Enable mid-song fires `OnSongEnter`; disable mid-song fires `OnSongExit`; nothing is missed
    (per-mod `inSong` tracked relative to *its own* activation).
- **Failure policy**:
  - `OnInitialize`/`OnEnabled` throw → **Faulted** (never runs again). `OnEnabled` must be
    strongly exception-safe, as `OnDisabled` is *not* called on a failed enable.
  - A tick hook (`OnTick`/`OnMenuTick`/`OnSongTick`/`OnSongEnter`) throws → the mod is faulted
    immediately and receives a best-effort `OnDisabled` revert.
- **Shutdown**: `OnSongExit`(if in song) → `OnDisabled`(if active) → `OnShutdown`, then the
  registry destroys the mod objects.

## Conflicts & resources

Some mods can't run together — e.g. **DropPedal** and **MIDI auto-tune** both drive tuning. They
express that by claiming the same named exclusive resource:

```cpp
std::vector<std::string_view> ClaimsExclusive() const override { return { "tuning-controller" }; }
int Priority() const override { return 10; }   // one GLOBAL priority per mod
```

Among all *enabled* mods claiming a resource the highest-`Priority()` one wins it; a mod that loses
any resource it claims is suppressed (its `OnDisabled` reverts its game state). The resolver
(`ConflictResolver.hpp`, pure and unit-tested) is deterministic greedy: order enabled mods by
`(Priority desc, Id asc)`, activate each iff none of its resources are already reserved by an
already-activated mod. `Tick` deactivates losers before activating winners, so a handoff of a
shared resource reverts the loser before the winner acquires.

## Settings

`WM_COPYDATA` runs on the message thread. The **GUI settings path** is queued (as a closure)
onto the registry instead of applied there; on the next `Tick` (MainThread) the registry
applies it, then notifies mods via `OnSettingsChanged`, so `IsEnabled`/`OnSettingsChanged` and
the legacy `ModManager` reads never race *that* writer. Non-settings GUI commands (Wwise,
Twitch, CrowdControl) still run immediately.

> The `Settings` maps are **not** globally thread-safe. Only the GUI/`WM_COPYDATA` writer was
> moved onto MainThread. Other legacy writers (CrowdControl, Twitch) still mutate settings from
> their own threads.
