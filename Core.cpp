#include "stdafx.h"
#include "Timer.h"
#include "Core.h"

ComPtr<ID3D12Device> Core::device;
ComPtr<IDXGIFactory4> Core::factory;
ComPtr<ID3D12Fence> Core::fence;

//RTV - render target view
UINT Core::rtvDescriptorSize = 0;
//DSV - depth/stencil view
UINT Core::dsvDescriptorSize = 0;
//CBV - constant buffer view
UINT Core::cbvDescriptorSize = 0;

D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Core::multiSamplingQualityLevels = {};

DXGI_FORMAT Core::pixelDefinitionFormat = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
DXGI_FORMAT Core::depthStencilFormat = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;


UINT Core::multiSamplingLevel = 4; //4xMSAA
UINT Core::multiSamplingQualityLevel = 0;

ComPtr<ID3D12GraphicsCommandList> Core::commandList;
ComPtr<ID3D12CommandQueue> Core::commandQueue;
ComPtr<ID3D12CommandAllocator> Core::commandAllocator;

UINT Core::displayWidth = 800;
UINT Core::displayHeight = 600;
UINT Core::refreshRate = 60; //60Hz

Core::BufferingType Core::buffering = Core::BufferingType::TRIPLE;

//Przyk�ad z ksi��ki si� wyk�ada wi�c wzi��em z przyk�ad�w MSDN
ComPtr<IDXGISwapChain1> Core::swapChain;

ComPtr<ID3D12DescriptorHeap> Core::rtvDescriptorHeap;
ComPtr<ID3D12DescriptorHeap> Core::dsvDescriptorHeap;

ComPtr<ID3D12Resource> Core::swapChainBackBuffers[Core::BufferingType::TRIPLE]; //Tymczasowo, zczai� jak to zainicjowa� w locie z konfiguracji
ComPtr<ID3D12Resource> Core::depthStencilBuffer;

INT Core::currentBackBuffer = 0;

HWND Core::mainWindow;
Core::RenderFunction Core::mainRenderFunction;
std::wstring Core::engineName = L"ENGINE";


void Core::createDevice() {

	HRESULT result = D3D12CreateDevice(
		NULL, //Bierzemy g��wny wy�wietlacz //Zrobi� by bra� pierwszy sprz�towy
		D3D_FEATURE_LEVEL_11_0, //Minimalny poziom jaki urz�dzenie musi ob�ugiwa�
		IID_PPV_ARGS(&Core::device) //Do tego ComPtr'a wrzu� ten device
		//Jest jeszcze 4 argument, kt�ry jest wska�nikiem na wska�nik zwracanego urz�dzenia
	);

	//Je�eli nie znajdzie interfacu urz�dzenia, szukaj adaptera softwarowego (Jak si� ma intela hd4000 to innej drogi nie ma)
	if (FAILED(result)) {
		UINT i = 0;
		ComPtr<IDXGIAdapter1> adapter;
		while (factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
			DXGI_ADAPTER_DESC1 desc1;
			adapter->GetDesc1(&desc1);
			if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			result = D3D12CreateDevice(
				adapter.Get(), 
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&Core::device)
			);

			if (SUCCEEDED(result)) {
				return;
			}

			++i;
		}

		ErrorUtils::messageAndExitIfFailed(
			result,
			L"B��d przy szukaniu adaptera programowego! Core::createDevice()",
			SOFTWARE_DEVICE_CREATION_ERROR
		);
	}
}

void Core::createSwapChainBuffersIntoRTVHeap() {
	for (UINT i = 0; i < Core::buffering; i++) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		ErrorUtils::messageAndExitIfFailed(
			swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBackBuffers[i])),
			L"B��d pobierania backBuffera!",
			GET_SWAPCHAIN_BACK_BUFFER_ERROR
		);

		device->CreateRenderTargetView(swapChainBackBuffers[i].Get(), NULL, rtvHeapHandle);
		//Zapami�tuje offset, to jest sterta po prostu zwyk�a
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
}

void Core::createDepthStencilView() {
	D3D12_RESOURCE_DESC depthStencilViewDesc;
	depthStencilViewDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Alignment = 0; //WTF??
	depthStencilViewDesc.Width = Core::displayWidth;
	depthStencilViewDesc.Height = Core::displayHeight;
	depthStencilViewDesc.DepthOrArraySize = 1; //WTF??
	depthStencilViewDesc.MipLevels = 1; //WTF??
	depthStencilViewDesc.Format = Core::depthStencilFormat;
	depthStencilViewDesc.SampleDesc.Count = 1;
	depthStencilViewDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilViewDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValueDefinition; //Zoptymalizowana
	clearValueDefinition.Format = Core::depthStencilFormat;
	clearValueDefinition.DepthStencil.Depth = 1.0f;
	clearValueDefinition.DepthStencil.Stencil = 0;
	
	ErrorUtils::messageAndExitIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&depthStencilViewDesc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
		&clearValueDefinition,
		IID_PPV_ARGS(Core::depthStencilBuffer.GetAddressOf())
	),
		L"B��d utworzenia depthStencilBuffera!",
		CREATE_DEPTH_STENCIL_BUFFER_ERROR
	);

	Core::device->CreateDepthStencilView(
		Core::depthStencilBuffer.Get(),
		NULL, //Je�eli resource ma typ (nie typeless) tutaj mo�e by� null, w przeciwnym razie trzeba wype�ni� struktur�. U nas ten typ to Core::pixelDefinitionFormat
		Core::getDSVHeapStartDescriptorHandle()
	);

	//Zmiana stanu zasobu (resource) z pocz�tkowego, na bycie depthBufferem
	Core::commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			Core::depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE
		)
	);
}

void Core::createViewPort() {
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float) Core::displayWidth;
	viewport.Height = (float) Core::displayHeight;
	viewport.MinDepth = 0; //To jest ten tyb buffora co si� ustawia. w naszym przypadku [0, 1], ale mo�e by� inny
	viewport.MaxDepth = 1;
	//The viewport needs to be reset whenever the command list is reset.
	Core::commandList->RSSetViewports(1, &viewport);
}

void Core::createScissorsRectangle() {
	RECT scissorsRectangle;
	scissorsRectangle.left = 0;
	scissorsRectangle.right = Core::displayWidth;
	scissorsRectangle.top = 0;
	scissorsRectangle.bottom = Core::displayHeight;

	Core::commandList->RSSetScissorRects(1, &scissorsRectangle);
}

D3D12_CPU_DESCRIPTOR_HANDLE Core::getRTVHeapStartDescriptorHandle() {
	return Core::rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
D3D12_CPU_DESCRIPTOR_HANDLE Core::getDSVHeapStartDescriptorHandle() {
	return Core::dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void Core::createRTVDescriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = Core::buffering;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.NodeMask = NULL;

	ErrorUtils::messageAndExitIfFailed(
		device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&Core::rtvDescriptorHeap)),
		L"B��d utworzenia sterty deskryptora RTV!",
		RTV_DESCRIPTOR_HEAP_CREATION_ERROR
	);
}

void Core::createDSVDescriptorHeap() {
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.NodeMask = NULL;

	ErrorUtils::messageAndExitIfFailed(
		device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&Core::dsvDescriptorHeap)),
		L"B��d utworzenia sterty deskryptora DSV!",
		DSV_DESCRIPTOR_HEAP_CREATION_ERROR
	);
}

void Core::createWindow() {
	Core::mainWindow = Core::initializeMainWindow(GetModuleHandle(NULL), engineName);
}

void Core::createSwapChain() {
	swapChain.Reset(); //Wypierdzielenie wska�nika kt�ry si� utworzy�
	
	//Poj�cia nie mam czemu przyk�ad z ksi��ki nie dzia�a
	//DXGI_MODE_DESC bufferDesc;
	//bufferDesc.Width = Core::displayWidth;
	//bufferDesc.Height = Core::displayHeight;
	//bufferDesc.RefreshRate.Numerator = Core::refreshRate;// 60 /
	//bufferDesc.RefreshRate.Denominator = 1;				 // 1
	//bufferDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	//bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	//bufferDesc.Format = Core::pixelDefinitionFormat;

	//DXGI_SAMPLE_DESC sampleDesc;
	//sampleDesc.Count = multiSamplingLevel == 0 ? 1 : multiSamplingLevel;
	//sampleDesc.Quality = multiSamplingQualityLevel == 0 ? 0 : (multiSamplingQualityLevel - 1); //Nie wiem czemu odejmowanie, ale spoko, si� sprawdzi jeszcze.

	//DXGI_SWAP_CHAIN_DESC swapChainDesc;
	//swapChainDesc.BufferDesc = bufferDesc;
	//swapChainDesc.SampleDesc = sampleDesc;
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//swapChainDesc.BufferCount = (UINT) Core::DOUBLE;
	//swapChainDesc.Windowed = true;
	//swapChainDesc.OutputWindow = Core::mainWindow;
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;



	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = Core::buffering;
	swapChainDesc.Width = Core::displayWidth;
	swapChainDesc.Height = Core::displayHeight;
	swapChainDesc.Format = Core::pixelDefinitionFormat;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	
	ErrorUtils::messageAndExitIfFailed(
		factory->CreateSwapChainForHwnd(
			Core::commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
			Core::mainWindow,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
		),
		L"B��d tworzenia swapChain'a!",
		SWAP_CHAIN_CREATION_ERROR
	);
}

void Core::createCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ErrorUtils::messageAndExitIfFailed(
		device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)),
		L"B��d tworzenia kolejki komend!",
		COMMAND_QUEUE_CREATION_ERROR
	);
}

void Core::createCommandAllocator() {
	ErrorUtils::messageAndExitIfFailed(
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
		L"B��d tworzenia alokatora komend!",
		COMMAND_ALLOCATOR_CREATION_ERROR
	);
}

void Core::createCommandList() {
	ErrorUtils::messageAndExitIfFailed(
		device->CreateCommandList(
			NULL,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator.Get(),
			NULL, //Poniewa� na razie nie b�dziemy rysowa� na ekranie
			IID_PPV_ARGS(&commandList)
		),
		L"B��d tworzenia alokatora komend!",
		COMMAND_ALLOCATOR_CREATION_ERROR
	);

	//Zamykamy, �eby zamkni�cie by�o stanem pocz�tkowym
	//Dzieje si� tak dlatego, �e na pocz�tku dodawania do list, b�dziemy wywo�ywa� na niej Reset()
	//a ona wymaga by lista by�a zamkni�ta.
	commandList->Close();
}

void Core::checkMultiSamplingSupport() {
	multiSamplingQualityLevels = {};
	multiSamplingQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	multiSamplingQualityLevels.Format = Core::pixelDefinitionFormat;
	multiSamplingQualityLevels.SampleCount = multiSamplingLevel; 
	multiSamplingQualityLevels.NumQualityLevels = 0; //Nie wiem czemu zero, sprawdzi� co to tak na prawd� znaczy

	ErrorUtils::messageAndExitIfFailed(
		device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&multiSamplingQualityLevels,
			sizeof(multiSamplingQualityLevels)
		),
		L"Twoja karta nie obs�uguje wymaganego multisamplingu! " + std::to_wstring(multiSamplingLevel) + L" :: Core::checkMultiSamplingSupport()",
		MULTISAMPLING_FEATURE_SUPPORT_ERROR
	);

	multiSamplingQualityLevel = multiSamplingQualityLevels.NumQualityLevels; //b�dzie 17, ale z docs�w wynika �e mo�na zmniejszy�, �eby zwi�kszy� wydajno��

	ErrorUtils::messageAndExitIfFalse(
		multiSamplingQualityLevel > 0,
		L"Nieoczekiwana warto�� MSAA Quality Level! " + std::to_wstring(multiSamplingQualityLevel) + L" :: Core::checkMultiSamplingSupport()",
		UNEXPECTED_MSAA_QUALITY_LEVEL_ERROR
	);
}

void Core::defineDescriptorSizes() {
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); // b�dzie 32
	dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV); // b�dzie 8
	//SRV - shader render view
	//UAV - unordered render view
	cbvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // b�dzie 32
}

void Core::createFence() {
	HRESULT result = Core::device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&Core::fence)	
	);

	ErrorUtils::messageAndExitIfFailed(
		result,
		L"B��d przy tworzeniu blokady urz�dzenia (Fence)! Core::createFence()",
		DEVICE_FENCE_CREATION_ERROR
	);
}

void Core::createFactory() {
	ErrorUtils::messageAndExitIfFailed(
		CreateDXGIFactory1(IID_PPV_ARGS(&Core::factory)),
		L"B��d przy tworzeniu fabryki! Core::createFactory()",
		FACTORY_CREATION_ERROR
	);
}

void Core::init(RenderFunction renderFunction) {
	Core::mainRenderFunction = renderFunction;
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ErrorUtils::messageAndExitIfFailed(
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)),
		L"B��d przej�cia w tryb debugowania! Core::init()",
		DEBUG_STATE_ERROR
	);
	debugController->EnableDebugLayer();
#endif
	createFactory();
	createDevice();
	createFence();
	defineDescriptorSizes();
	checkMultiSamplingSupport();
	createWindow();
	createCommandQueue();
	createCommandAllocator();
	createCommandList();
	createSwapChain();
	createRTVDescriptorHeap();
	createDSVDescriptorHeap();
	createSwapChainBuffersIntoRTVHeap();
	createDepthStencilView();
	createViewPort();
	createScissorsRectangle();
	Timer::init();
}

void Core::calculateFrameStats() {
	// Code computes the average frames per second, and also the�
	// average time it takes to render one frame. These stats�
	// are appended to the window caption bar.

	static int frameCounter = 0;
	static float timeElapsed = 0.0f;

	frameCounter++;

	// Compute averages over one second period.
	float test = Timer::getTotalTime();
	if ((Timer::getTotalTime() - timeElapsed) >= 1.0f) {

		std::wstring windowText = Core::engineName +
			L"|��fps: " + std::to_wstring((float) frameCounter) +
			L"��mspf: " + std::to_wstring(1000.0f / (float) frameCounter);

		SetWindowText(Core::mainWindow, windowText.c_str());

		// Reset for next average.
		frameCounter = 0;
		timeElapsed += 1.0f;
	}
}

HWND Core::initializeMainWindow(HINSTANCE hInstance, std::wstring &windowName, bool showcmd) {
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

	registationStructure.lpfnWndProc = Core::messagesHandler; //Wska�nik do procedury obs�uguj�cej okno, koniecznie __stdcall;
	registationStructure.lpszMenuName = NULL;
	registationStructure.lpszClassName = windowName.c_str();

	registationStructure.style = CS_HREDRAW | CS_VREDRAW; // style klasy https://msdn.microsoft.com/pl-pl/library/windows/desktop/ff729176(v=vs.85).aspx

	ErrorUtils::messageAndExitIfFalse(
		RegisterClassEx(&registationStructure), //Pr�ba rejestracji naszego okna w systemie
		L"Odmowa rejestracji okna w systemie!",
		WINDOW_REGISTATION_ERROR
	);


	HWND windowHandler = CreateWindowEx(
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


LRESULT CALLBACK Core::messagesHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

void Core::windowMainLoopHandler() {
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
			Timer::tick();
			Core::calculateFrameStats();
			Core::mainRenderFunction();
		}
	}
}