// Engine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main() {

	//Core::init();
	std::wstring s = L"Nie";
	HINSTANCE hisnstance = GetModuleHandle(0);
	HWND hwnd = initializeMainWindow(hisnstance, s);
	Core::init();
    return 0;
}

