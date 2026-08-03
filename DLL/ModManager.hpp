#pragma once

#include "QualityOfLife.hpp"
#include "Mods/VolumeControl.hpp"
#include "Mods/AudioDevices.hpp"
#include "Mods/BugPrevention.hpp"
#include "Keyboard.hpp"
#include "Mods/Loft.hpp"
#include "Keybindings.hpp"
#include "Mods/ExtendedRangeMode.hpp"
#include "Mods/CustomSongTitles.hpp"
#include "D3DInfo.h"
#include <string>

struct GameLoopState {
    bool skipERSleep = false; // If using RR past 100%, remove the 1.5s sleep on ER mode, to stop flickering colors.
    bool forkInToasterNewProfile = false; // If Auto Load Profile has a specified profile, and we can't find it, then this will be true.
    bool automatedSongTimer = false; // If true, we will always show the song timer.
    bool guitarSpeakPresent = false; // If true, read the notes inputted and press the key combo provided. (True - On, False - Off)
    std::string selectedUser;
};

namespace ModManager {
    inline const double DefaultNSPTimeLimit = 10.9899997711182; // The default time for NSP.

    void InitializeConfiguration();
    void InitializeMods(bool debug);
    void ApplyStartupMods();
    void HandlePostGameLoadedMods(GameLoopState& state);
    void UpdateGameLoadingState(GameLoopState& state);

    void EnableRiffRepeaterFeatures();
    void LogSongIDForRiffRepeater();
    void EnableRiffRepeaterFeatures();
    void HandleMidiAutoTuningInSong();
    void HandleSongTimerDisplay(GameLoopState& state);
    void HandleAutoLoadProfile(GameLoopState& state);
    void HandleSpecificProfileLoad(GameLoopState& state);
    void HandleAlwaysOnMods(GameLoopState& state);
    void HandleInMenuState(GameLoopState& state);
    void HandleInSongState(GameLoopState& state);
    void HandleTwoRTCBypassToggle();
    void HandleNonStopPlayTimer();
    void HandleLinearRiffRepeaterToggle();
    void HandleMidiDeviceScanning();
    void HandleMenuFeatures(GameLoopState& state);
    void HandleHeadstockCacheReset(GameLoopState& state);
    void CleanupSongSpecificStates(GameLoopState& state);
    void ConfigureAlternativeSampleRate();
    void CheckIfGameHasLoaded();
    void ApplyAudioDeviceConfiguration();
    void HandleMicrophoneVolumeOverride();
    void HandleAudioBackgroundToggle();
    void HandleTwoRTCBypassToggle();
}