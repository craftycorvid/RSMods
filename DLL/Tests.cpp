#include "stdafx.h"
#include "Tests.hpp"

namespace Tests
{
	/// <summary>
	/// Did we enter the song?
	/// </summary>
	bool EnterTestSong()
	{
		// User enters main menu
		Sleep(3500);

		// Enter LAS menu
		LOG_INFO("Entering LAS" << std::endl);
		PressKey(VK_RETURN);
		
		// Wait for LAS menu animation
		Sleep(2500);

		// Select the song
		LOG_INFO("Entering song" << std::endl);
		PressKey(VK_RETURN);

		// Wait for the animation
		Sleep(3500);

		// Enter the tuner
		LOG_INFO("Entering tuner" << std::endl);
		PressKey(VK_RETURN);

		// Wait for the animation
		Sleep(3000);

		// Enter tuner skip menu
		LOG_INFO("Opening skip tuner menu" << std::endl);
		PressKey(VK_DELETE);

		// Wait for the animation
		Sleep(1000);
		
		// Skip the tuner
		LOG_INFO("Skipping tuner" << std::endl);
		PressKey(VK_RETURN);

		// Sleep till we enter the song
		Sleep(5000);

		// Return if we are in a song or not.
		return GameState::IsInSong();
	}


	/// <summary>
	/// Presses the key specified.
	/// </summary>
	void PressKey(int Key)
	{
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYDOWN, Key, 0);
		Sleep(30);
		PostMessage(FindWindow(NULL, L"Rocksmith 2014"), WM_KEYUP, Key, 0);
	}
}