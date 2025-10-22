#pragma once

#include <string>

/// <summary>
/// EventName, Text.
/// </summary>
struct VoiceOver {
	std::string EventName;
	std::string Text;

	VoiceOver() = default;

	VoiceOver(std::string_view const& eventName, std::string_view const& text) : EventName(eventName), Text(text) {}
};