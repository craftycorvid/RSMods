#pragma once

namespace GuitarSpeak {
	byte GetCurrentNote();
	std::string GetCurrentNoteName();
	bool RunGuitarSpeak();
	void FillKeyList();

	inline bool verbose = false;

	inline int lastNote;
	inline int lastNoteBuffer = 0xFF, currentNoteBuffer = 0xFF; // 0xFF = End Note
	inline int timer = 50; // Milliseconds | Original value of 50ms
	inline bool sendKeystrokesToRS2014 = true, newNote = false;
	inline std::string* strKeyList = new std::string[96]; // 12 notes in an octave, 8 octaves spanned (-1 <-> 6) || Limit C-1 <-> C7
	inline int noNote = 0x0, endOfNote = 0xFF; // Used so we can call these instead of 0xFF and 0x0 in the file.

	inline std::map<std::string, int> keyToVKey{
		{"DELETE", VK_DELETE},	// Delete
		{"SPACE", VK_SPACE},	// Space
		{"ENTER", VK_RETURN},	// Enter
		{"TAB", VK_TAB},		// Tab
		{"PGUP", VK_PRIOR},		// Page Up
		{"PGDN", VK_NEXT},		// Page Down
		{"UP", VK_UP},			// Up Arrow
		{"DOWN", VK_DOWN},		// Down Arrow
		{"ESCAPE", VK_ESCAPE},	// Escape
		{"OBRACKET", VK_OEM_4}, // Open Bracket
		{"CBRACKET", VK_OEM_6}, // Close Bracket
		{"TILDEA", VK_OEM_3},	// Tilde/a
		{"FORSLASH", VK_OEM_2}, // Forward Slash
		{"ALT", VK_MENU},        // Alt
	};

	inline std::vector<std::string> noteLetters = {
		"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
	};
};