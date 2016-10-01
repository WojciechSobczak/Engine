// Engine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <functional>


int main() {
	try {
		Core::init([]() {
			Renderer::drawSomething();
		});
		Core::start();
		printf("Coœ");
	}
	catch (DxException& e) {
		MessageBox(nullptr, e.toString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
	
    return 0;
}

