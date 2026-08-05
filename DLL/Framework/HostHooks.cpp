#include <iomanip>

#include "HostHooks.hpp"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "../Log.hpp"

namespace Framework::Hooks {
	struct RenderHooks::Impl {
		struct Entry {
			const IMod* mod;
			EndSceneFn fn;
		};

		struct DispatchBatch {
			std::shared_ptr<const std::vector<Entry>> entries;
			std::shared_ptr<const std::unordered_set<const IMod*>> active;
			std::vector<const IMod*> counted;
		};

		void PublishEntries() {
			entriesSnapshot = std::make_shared<const std::vector<Entry>>(entries);
		}

		void PublishActive() {
			activeSnapshot = std::make_shared<const std::unordered_set<const IMod*>>(active);
		}

		DispatchBatch BeginDispatch() {
			std::lock_guard<std::mutex> lock(mutex);
			DispatchBatch batch{ entriesSnapshot, activeSnapshot, {} };

			if (batch.entries && batch.active) {
				for (const auto& entry : *batch.entries) {
					if (batch.active->count(entry.mod) &&
						std::find(batch.counted.begin(), batch.counted.end(), entry.mod) == batch.counted.end()) {
						batch.counted.push_back(entry.mod);
						++inFlightByMod[entry.mod];
					}
				}
			}

			return batch;
		}

		void EndDispatch(const std::vector<const IMod*>& counted) {
			std::lock_guard<std::mutex> lock(mutex);

			for (const IMod* mod : counted) {
				const auto it = inFlightByMod.find(mod);

				if (it != inFlightByMod.end() && --it->second <= 0) {
					inFlightByMod.erase(it);
				}
			}
		}

		void NoteRenderFault(const IMod* mod) {
			std::lock_guard<std::mutex> lock(mutex);

			if (faultedMods.insert(mod).second) {
				LOG_ERROR("[Framework] a render callback threw; its mod will be disabled" << std::endl);
			}
		}

		std::vector<Entry> entries;
		std::unordered_set<const IMod*> active;
		std::unordered_map<const IMod*, int> inFlightByMod;
		std::unordered_set<const IMod*> faultedMods;
		std::shared_ptr<const std::vector<Entry>> entriesSnapshot;
		std::shared_ptr<const std::unordered_set<const IMod*>> activeSnapshot;
		std::mutex mutex;
	};

	RenderHooks::RenderHooks() : impl(std::make_unique<Impl>()) {}
	RenderHooks::~RenderHooks() = default;

	void RenderHooks::Subscribe(const IMod* mod, EndSceneFn fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		impl->entries.push_back({ mod, std::move(fn) });
		impl->PublishEntries();
	}

	void RenderHooks::RemoveMod(const IMod* mod) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		const auto before = impl->entries.size();
		impl->entries.erase(std::remove_if(impl->entries.begin(), impl->entries.end(),
			[mod](const Impl::Entry& entry) { return entry.mod == mod; }), impl->entries.end());

		if (impl->entries.size() != before) impl->PublishEntries();
		if (impl->active.erase(mod)) impl->PublishActive();

		impl->inFlightByMod.erase(mod);
		impl->faultedMods.erase(mod);
	}

	void RenderHooks::SetModActive(const IMod* mod, bool isActive) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		const bool changed = isActive
			? impl->active.insert(mod).second
			: impl->active.erase(mod) > 0;

		if (changed) impl->PublishActive();
	}

	bool RenderHooks::IsModQuiescent(const IMod* mod) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		const auto it = impl->inFlightByMod.find(mod);
		return it == impl->inFlightByMod.end() || it->second <= 0;
	}

	std::vector<const IMod*> RenderHooks::TakeFaultedMods() {
		std::lock_guard<std::mutex> lock(impl->mutex);
		
		std::vector<const IMod*> mods(impl->faultedMods.begin(), impl->faultedMods.end());
		impl->faultedMods.clear();

		return mods;
	}

	void RenderHooks::DispatchEndScene(IDirect3DDevice9* device) {
		auto batch = impl->BeginDispatch();

		struct InFlightGuard {
			Impl* impl;
			const std::vector<const IMod*>& counted;
			~InFlightGuard() { impl->EndDispatch(counted); }
		} guard{ impl.get(), batch.counted };

		if (batch.counted.empty()) return;

		for (const auto& entry : *batch.entries) {
			if (batch.active->find(entry.mod) == batch.active->end()) continue;

			try {
				entry.fn(device);
			}
			catch (...) {
				impl->NoteRenderFault(entry.mod);
			}
		}
	}

	RenderHooks& Render() {
		static RenderHooks instance;
		return instance;
	}

	void DispatchEndScene(IDirect3DDevice9* device) {
		Render().DispatchEndScene(device);
	}
}
