#include "stdafx.h"
#include "window.h"

HWND initializeMainWindow(HINSTANCE hInstance, std::wstring &windowName, bool showcmd) {
	//Struktura s³u¿¹ca do rejestracji okna w systemie
	WNDCLASSEX registationStructure;
	registationStructure.cbSize = sizeof(WNDCLASSEX); //Rozmiar struktury w bajtach. Nale¿y tu wpisaæ sizeof (WINDOWCLASSEX).
	registationStructure.cbWndExtra = 0; //Dodatkowe bajty pamiêci dla klasy (mo¿na ustawiæ na 0)
	registationStructure.cbClsExtra = 0; //@up

	registationStructure.hInstance = hInstance; //Identyfikator aplikacji, która ma byæ w³aœcicielem okna
	registationStructure.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Du¿a ikonka, widaæ j¹ kiedy naciœniesz Alt-Tab; LoadIcon(NULL, IDI_APPLICATION) ³aduje domyœln¹
	registationStructure.hIconSm = LoadIcon(NULL, IDI_APPLICATION); //Ma³a ikonka naszej aplikacji. Widaæ j¹ w rogu naszego okienka oraz na pasku zadañ. 
	registationStructure.hCursor = LoadCursor(NULL, IDC_ARROW); //Kursor myszki. Analogicznie, jak dla ikonki, korzystamy z LoadCursor(NULL, IDC_ARROW);
	registationStructure.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1); //T³o naszego okienka, czyli jego kolor i wzór. Wybieramy domyœlne, czyli zwykle szare t³o - (HBRUSH)(COLOR_WINDOW + 1).

	registationStructure.lpfnWndProc = messagesHandler; //WskaŸnik do procedury obs³uguj¹cej okno, koniecznie __stdcall;
	registationStructure.lpszMenuName = NULL;
	registationStructure.lpszClassName = windowName.c_str();

	registationStructure.style = CS_HREDRAW | CS_VREDRAW; // style klasy https://msdn.microsoft.com/pl-pl/library/windows/desktop/ff729176(v=vs.85).aspx

	ErrorUtils::messageAndExitIfFalse(
		RegisterClassEx(&registationStructure), //Próba rejestracji naszego okna w systemie
		L"Odmowa rejestracji okna w systemie!",
		WINDOW_REGISTATION_ERROR
	);


	HWND windowHandler = CreateWindowEx (
		WS_EX_WINDOWEDGE, //Rozszerzone parametry stylu okna. Damy sobie WS_EX_WINDOWEDGE, czyli trójwymiarow¹ ramkê
		windowName.c_str(), //Nazwa klasy okna, któr¹ w³aœnie zarejestrowaliœmy
		windowName.c_str(), //Napis na pasku tytu³owym okienka
		WS_OVERLAPPEDWINDOW, //Style okienka
		0, //X po³o¿enia pocz¹tkowego okienka
		0, //Y po³o¿enia pocz¹tkowego okienka
		Core::getDisplayWidth(), //Szerokoœæ okienka
		Core::getDisplayHeight(), //Wysokoœæ okienka
		NULL, //Uchwyt okna rodzicielskiego (nadrzêdnego). 
		NULL, //Uchwyt menu dla naszego okna.
		hInstance, //Uchwyt aplikacji, której przypisujemy okienko. Dajemy parametr hInstance, otrzymany od systemu jako argument dla WinMain
		NULL //Teoretycznie jest to wskaŸnik do dodatkowych parametrów
	 );

	ErrorUtils::messageAndExitIfTrue(
		windowHandler == NULL,
		L"B³¹d utworzenia okna!",
		WINDOW_CREATION_ERROR
	);

	ShowWindow(windowHandler, showcmd);

	//Przerysowanie dla pewnoœci
	UpdateWindow(windowHandler);
	return windowHandler;
}


LRESULT CALLBACK messagesHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_CLOSE: {
			DestroyWindow(hwnd);
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		default: {
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
	return 0;
}

void mainLoop(void (*function) (void)) {
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (true) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				exit(0);
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			function();
		}
	}
}
