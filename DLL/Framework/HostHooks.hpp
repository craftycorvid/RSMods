#pragma once

#include <functional>
#include <memory>
#include <vector>

struct IDirect3DDevice9;

namespace Framework {
	class IMod;
}

namespace Framework::Hooks {
	using EndSceneFn = std::function<void(IDirect3DDevice9*)>;

	class RenderHooks {
	public:
		RenderHooks();
		~RenderHooks();

		RenderHooks(const RenderHooks&) = delete;
		RenderHooks& operator=(const RenderHooks&) = delete;

		void Subscribe(const IMod* mod, EndSceneFn fn);
		void RemoveMod(const IMod* mod);
		void SetModActive(const IMod* mod, bool active);
		bool IsModQuiescent(const IMod* mod);
		std::vector<const IMod*> TakeFaultedMods();
		void DispatchEndScene(IDirect3DDevice9* device);

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	RenderHooks& Render();
	void DispatchEndScene(IDirect3DDevice9* device);
}
