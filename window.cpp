#include "stdafx.h"
#include "window.h"

HWND initializeMainWindow(HINSTANCE hInstance, std::wstring &windowName, bool showcmd) {
	//Struktura s�u��ca do rejestracji okna w systemie
	WNDCLASSEX registationStructure;
	registationStructure.cbSize = sizeof(WNDCLASSEX); //Rozmiar struktury w bajtach. Nale�y tu wpisa� sizeof (WINDOWCLASSEX).
	registationStructure.cbWndExtra = 0; //Dodatkowe bajty pami�ci dla klasy (mo�na ustawi� na 0)
	registationStructure.cbClsExtra = 0; //@up

	registationStructure.hInstance = hInstance; //Identyfikator aplikacji, kt�ra ma by� w�a�cicielem okna
	registationStructure.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Du�a ikonka, wida� j� kiedy naci�niesz Alt-Tab; LoadIcon(NULL, IDI_APPLICATION) �aduje domy�ln�
	registationStructure.hIconSm = LoadIcon(NULL, IDI_APPLICATION); //Ma�a ikonka naszej aplikacji. Wida� j� w rogu naszego okienka oraz na pasku zada�. 
	registationStructure.hCursor = LoadCursor(NULL, IDC_ARROW); //Kursor myszki. Analogicznie, jak dla ikonki, korzystamy z LoadCursor(NULL, IDC_ARROW);
	registationStructure.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1); //T�o naszego okienka, czyli jego kolor i wz�r. Wybieramy domy�lne, czyli zwykle szare t�o - (HBRUSH)(COLOR_WINDOW + 1).

	registationStructure.lpfnWndProc = messagesHandler; //Wska�nik do procedury obs�uguj�cej okno, koniecznie __stdcall;
	registationStructure.lpszMenuName = NULL;
	registationStructure.lpszClassName = windowName.c_str();

	registationStructure.style = CS_HREDRAW | CS_VREDRAW; // style klasy https://msdn.microsoft.com/pl-pl/library/windows/desktop/ff729176(v=vs.85).aspx

	ErrorUtils::messageAndExitIfFalse(
		RegisterClassEx(&registationStructure), //Pr�ba rejestracji naszego okna w systemie
		L"Odmowa rejestracji okna w systemie!",
		WINDOW_REGISTATION_ERROR
	);


	HWND windowHandler = CreateWindowEx (
		WS_EX_WINDOWEDGE, //Rozszerzone parametry stylu okna. Damy sobie WS_EX_WINDOWEDGE, czyli tr�jwymiarow� ramk�
		windowName.c_str(), //Nazwa klasy okna, kt�r� w�a�nie zarejestrowali�my
		windowName.c_str(), //Napis na pasku tytu�owym okienka
		WS_OVERLAPPEDWINDOW, //Style okienka
		0, //X po�o�enia pocz�tkowego okienka
		0, //Y po�o�enia pocz�tkowego okienka
		Core::getDisplayWidth(), //Szeroko�� okienka
		Core::getDisplayHeight(), //Wysoko�� okienka
		NULL, //Uchwyt okna rodzicielskiego (nadrz�dnego). 
		NULL, //Uchwyt menu dla naszego okna.
		hInstance, //Uchwyt aplikacji, kt�rej przypisujemy okienko. Dajemy parametr hInstance, otrzymany od systemu jako argument dla WinMain
		NULL //Teoretycznie jest to wska�nik do dodatkowych parametr�w
	 );

	ErrorUtils::messageAndExitIfTrue(
		windowHandler == NULL,
		L"B��d utworzenia okna!",
		WINDOW_CREATION_ERROR
	);

	ShowWindow(windowHandler, showcmd);

	//Przerysowanie dla pewno�ci
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
