#include "stdafx.h"
#include <sstream>
#include "ErrorUtils.h"

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber) {}

std::wstring DxException::toString()const {
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

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

void ErrorUtils::throwIfFailed(HRESULT result) {
	
}
