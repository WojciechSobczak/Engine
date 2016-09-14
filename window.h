#pragma once
#include "stdafx.h"

HWND initializeMainWindow(HINSTANCE hInstance, std::wstring &windowName, bool showcmd = true);

//LRESULT = long
//CALLBACK = __stdcall
LRESULT CALLBACK messagesHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void mainLoop(void (*function) (void));