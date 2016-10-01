#pragma once
#include "stdafx.h"
#include <comdef.h>

#define DEBUG_STATE_ERROR -1
#define HARDWARE_DEVICE_CREATION_ERROR -2
#define SOFTWARE_DEVICE_CREATION_ERROR -3
#define FACTORY_CREATION_ERROR -4
#define DEVICE_FENCE_CREATION_ERROR -5
#define MULTISAMPLING_FEATURE_SUPPORT_ERROR -6
#define UNEXPECTED_MSAA_QUALITY_LEVEL_ERROR -7
#define COMMAND_QUEUE_CREATION_ERROR -8
#define COMMAND_ALLOCATOR_CREATION_ERROR -9
#define COMMAND_LIST_CREATION_ERROR -10
#define SWAP_CHAIN_CREATION_ERROR -11
#define RTV_DESCRIPTOR_HEAP_CREATION_ERROR -12
#define DSV_DESCRIPTOR_HEAP_CREATION_ERROR -13
#define GET_SWAPCHAIN_BACK_BUFFER_ERROR -14
#define CREATE_DEPTH_STENCIL_BUFFER_ERROR -15

//Po inicjalizacjach obni¿yæ
#define WINDOW_REGISTATION_ERROR -250 
#define WINDOW_CREATION_ERROR -251 

namespace ErrorUtils {
	//Funkcja ta podczas wykrycia b³êdu wyrzuci odpowiednie okienko z wiadomoœci¹, oraz zamknie program z podanym kodem b³êdu.
	void messageAndExitIfFailed(HRESULT result, std::wstring message, unsigned int codeOfError);
	void messageAndExitIfTrue(bool result, std::wstring message, unsigned int codeOfError);
	void messageAndExitIfFalse(bool result, std::wstring message, unsigned int codeOfError);
	void throwIfFailed(HRESULT result);
	inline std::wstring AnsiToWString(const std::string& str) {
		WCHAR buffer[512];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
		return std::wstring(buffer);
	}
}

class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring toString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) { \
	HRESULT hr__ = (x); \
	std::wstring wfn = ErrorUtils::AnsiToWString(__FILE__); \
	if(FAILED(hr__)) { \
		throw DxException(hr__, L#x, wfn, __LINE__); \
	} \
}
#endif

