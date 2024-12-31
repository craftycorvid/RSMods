#pragma once

namespace AudioDevices {
	void SetupMicrophones();
	void SetMicrophoneVolume(std::string microphoneName, int volume);
	int GetMicrophoneVolume(std::string microphoneName);
	void ChangeOutputSampleRate();

	inline std::map<std::string, LPWSTR> activeMicrophones;

	inline int output_SampleRate = 48000;
};