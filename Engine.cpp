// Engine.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void nic() {
	Sleep(1);
}

int main() {
	Core::init();
	printf("Coœ");
	mainLoop(nic);
    return 0;
}

