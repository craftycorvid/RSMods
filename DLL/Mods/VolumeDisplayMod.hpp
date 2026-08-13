#pragma once

#include "../Framework/Framework.hpp"

class VolumeDisplayMod : public Framework::IMod {
public:
	MOD_ID(VolumeDisplayMod)

	void OnInitialize(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	static void ToggleMute(bool player2);
	static void ChangeVolume(const Framework::ModContext& c, const Framework::KeyEvent& event,
		std::string_view channel, int overlayIndex);
	void SyncDisplay(Framework::ModContext& c);
	bool MoreThanThreeSecondsPassed() const;

	struct VolumeBinding {
		const char* key;
		const char* channel;
		int overlayIndex;
	};

	static constexpr std::array<VolumeBinding, 7> volumeBindings = { {
		{ "MasterVolumeKey", "Master_Volume", 0 },
		{ "SongVolumeKey", "Mixer_Music", 1 },
		{ "Player1VolumeKey", "Mixer_Player1", 2 },
		{ "Player2VolumeKey", "Mixer_Player2", 3 },
		{ "MicrophoneVolumeKey", "Mixer_Mic", 4 },
		{ "VoiceOverVolumeKey", "Mixer_VO", 5 },
		{ "SFXVolumeKey", "Mixer_SFX", 6 },
	} };
};
