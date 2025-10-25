// DLL Hijack. Do NOT remove this line!
#include "Proxy.hpp"
#include "D3DInfo.h"
#include "Tests.hpp"
#include "CC/ControlServer.hpp"
#include "Mods/Enumeration.hpp"
#include "Mods/CustomSongTitles.hpp"
#include "Mods/CollectColors.hpp"
#include "Mods/GuitarSkeletons.hpp"
#include "Mods/GuitarSpeak.hpp"
#include "Mods/DisableControllers.hpp"
#include "Mods/Midi.hpp"
#include "Mods/VolumeControl.hpp"
#include "Mods/LaunchOnExternalMonitor.hpp"
#include "Mods/VoiceOverControl.hpp"
#include "Mods/RiffRepeater.hpp"
#include "Mods/AudioDevices.hpp"
#include "Mods/BugPrevention.hpp"
#include "Mods/TrueTuning.hpp"
#include "Mods/Loft.hpp"
#include "Menu.hpp"
#include "Keyboard.hpp"

std::vector<std::string> effectQueue;
std::vector<std::string> enabledEffects;

bool saveNewRRSpeedToFile = false;
inline bool ImGuiInit = false; // Has ImGui already been init? If we close the game with this being false, then we get an assert.
inline const double DefaultNSPTimeLimit = 10.9899997711182; // The default time for NSP.


// Looping functionality.
inline float loopStart = NULL; // The start of the loop, as specified by the user.
inline float roughLoopStart = NULL; // Just like loopStart, except we account for the lead-in time.
inline float loopEnd = NULL; // The end of the loop, as specified by the user.

inline std::string selectedUser = "";

bool Contains(std::string str, const char* key){ return str.find(key) != std::string::npos; }