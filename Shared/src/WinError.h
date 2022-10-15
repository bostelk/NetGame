#pragma once
#include <string>

// Captures Winsock error code.
struct WinError {
	int code;

	WinError();

	// Returns a message for error code.
	std::string GetMessageA();

	// Returns a formated string: (Code) Messsage.
	std::string to_string();
};