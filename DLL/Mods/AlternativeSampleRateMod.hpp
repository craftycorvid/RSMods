#pragma once

#include "../Framework/Framework.hpp"

// Forces the audio engine's sample-rate buffer markers during boot when the user runs an alternative
// output sample rate. Loading-phase only (the engine only reads them while coming up) and apply-only.
class AlternativeSampleRateMod : public Framework::IMod {
public:
	std::string_view Id() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnTick(Framework::ModContext& c) override;
};
