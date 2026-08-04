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

namespace ModManager {

    void InitializeConfiguration();
    void InitializeMods(bool debug);
    void ApplyStartupMods();
    void HandlePostGameLoadedMods();
    void UpdateGameLoadingState();

    void CheckIfGameHasLoaded();
    void ApplyAudioDeviceConfiguration();
}