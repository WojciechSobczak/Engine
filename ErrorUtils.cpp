#include "stdafx.h"
#include <sstream>
#include "ErrorUtils.h"


void ErrorUtils::messageAndExitIfFailed(HRESULT result, std::wstring message, unsigned int codeOfError) {
	if (FAILED(result)) {
		std::wstringstream stream;
		stream << std::hex << result;
		std::wstring prompt = message + L" | HRESULT: " + stream.str();
		MessageBox(NULL, prompt.c_str(), L"Wyst¹pi³ nieoczekiwany b³¹d!", MB_OK | MB_ICONERROR);
		exit(codeOfError);
	}
}

void ErrorUtils::messageAndExitIfFalse(bool result, std::wstring message, unsigned int codeOfError) {
	ErrorUtils::messageAndExitIfTrue(!result, message, codeOfError);
}

void ErrorUtils::messageAndExitIfTrue(bool result, std::wstring message, unsigned int codeOfError) {
	if (result) {
		MessageBox(NULL, message.c_str(), L"Wyst¹pi³ nieoczekiwany b³¹d!", MB_OK | MB_ICONERROR);
		exit(codeOfError);
	}
}
