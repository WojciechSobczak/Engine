// Engine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <functional>


int main() {
	Core::init([]() {
		Sleep(1);
	});
	Core::start();
	printf("Coœ");
    return 0;
}

