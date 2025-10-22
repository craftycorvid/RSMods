#pragma once

#include <string>
#include <vector>
#include <map>

using tuningFunction = void (*)(int highestTuning, float TrueTuning_Hertz);

/// <summary>
/// pedalName, CC_Bank, PC_Channel, CC_Channel, supportsDropTuning, supportsTrueTuning, semiTones, activeBypassMap, autoTuneFunction, softwarePedal = false
/// </summary>
struct MidiPedal {
    std::string pedalName = "DUMMY PEDAL";
    char        CC_Bank = 0;
    char        PC_Channel = 0;
    char        CC_Channel = 0;
    bool        supportsDropTuning = false;
    bool        supportsTrueTuning = false;
    std::vector<float> semiTones = {};
    std::map<char, char> activeBypassMap = {};
    tuningFunction autoTuneFunction = nullptr;
    bool        softwarePedal = false;

    MidiPedal() = default;

    MidiPedal(std::string _pedalName, char _CC_Bank, char _PC_Channel,
        char _CC_Channel, bool _supportsDropTuning, bool _supportsTrueTuning,
        std::vector<float> _semiTones, std::map<char, char> _activeBypassMap,
        tuningFunction _tuningFunction, bool _softwarePedal = false)
        : pedalName(std::move(_pedalName)),
        CC_Bank(_CC_Bank),
        PC_Channel(_PC_Channel),
        CC_Channel(_CC_Channel),
        supportsDropTuning(_supportsDropTuning),
        supportsTrueTuning(_supportsTrueTuning),
        semiTones(std::move(_semiTones)),
        activeBypassMap(std::move(_activeBypassMap)),
        autoTuneFunction(_tuningFunction),
        softwarePedal(_softwarePedal)
    {}
};