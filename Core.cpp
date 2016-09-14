#include "stdafx.h"
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

UINT Core::multiSamplingLevel = 4; //4xMSAA
UINT Core::multiSamplingQualityLevel = 0;

ComPtr<ID3D12GraphicsCommandList> Core::commandList;
ComPtr<ID3D12CommandQueue> Core::commandQueue;
ComPtr<ID3D12CommandAllocator> Core::commandAllocator;

UINT Core::displayWidth = 800;
UINT Core::displayHeight = 600;
UINT Core::refreshRate = 60; //60Hz

Core::BufferingType Core::buffering = Core::BufferingType::TRIPLE;

//Przyk³ad z ksi¹¿ki siê wyk³ada wiêc wzi¹³em z przyk³adów MSDN
ComPtr<IDXGISwapChain1> Core::swapChain;

ComPtr<ID3D12DescriptorHeap> Core::rtvDescriptorHeap;
ComPtr<ID3D12DescriptorHeap> Core::dsvDescriptorHeap;

ComPtr<ID3D12Resource> Core::swapChainBackBuffers[Core::BufferingType::TRIPLE]; //Tymczasowo, zczaiæ jak to zainicjowaæ w locie z konfiguracji

INT Core::currentBackBuffer = 0;

HWND Core::mainWindow;
std::wstring Core::engineName = L"ENGINE";


void Core::createDevice() {

	HRESULT result = D3D12CreateDevice(
		NULL, //Bierzemy g³ówny wyœwietlacz //Zrobiæ by bra³ pierwszy sprzêtowy
		D3D_FEATURE_LEVEL_11_0, //Minimalny poziom jaki urz¹dzenie musi ob³ugiwaæ
		IID_PPV_ARGS(&Core::device) //Do tego ComPtr'a wrzuæ ten device
		//Jest jeszcze 4 argument, który jest wskaŸnikiem na wskaŸnik zwracanego urz¹dzenia
	);

	//Je¿eli nie znajdzie interfacu urz¹dzenia, szukaj adaptera softwarowego (Jak siê ma intela hd4000 to innej drogi nie ma)
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
			L"B³¹d przy szukaniu adaptera programowego! Core::createDevice()",
			SOFTWARE_DEVICE_CREATION_ERROR
		);
	}
}

void Core::createSwapChainBuffersIntoRTVHeap() {
	for (UINT i = 0; i < Core::buffering; i++) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		ErrorUtils::messageAndExitIfFailed(
			swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBackBuffers[i])),
			L"B³¹d pobierania backBuffera!",
			GET_SWAPCHAIN_BACK_BUFFER_ERROR
		);

		device->CreateRenderTargetView(swapChainBackBuffers[i].Get(), NULL, rtvHeapHandle);
		//Zapamiêtuje offset, to jest sterta po prostu zwyk³a
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
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
		L"B³¹d utworzenia sterty deskryptora RTV!",
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
		L"B³¹d utworzenia sterty deskryptora DSV!",
		DSV_DESCRIPTOR_HEAP_CREATION_ERROR
	);
}

void Core::createWindow() {
	Core::mainWindow = initializeMainWindow(GetModuleHandle(NULL), engineName);
}

void Core::createSwapChain() {
	swapChain.Reset(); //Wypierdzielenie wskaŸnika który siê utworzy³
	
	//Pojêcia nie mam czemu przyk³ad z ksi¹¿ki nie dzia³a
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
	//sampleDesc.Quality = multiSamplingQualityLevel == 0 ? 0 : (multiSamplingQualityLevel - 1); //Nie wiem czemu odejmowanie, ale spoko, siê sprawdzi jeszcze.

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
		L"B³¹d tworzenia swapChain'a!",
		SWAP_CHAIN_CREATION_ERROR
	);
}

void Core::createCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ErrorUtils::messageAndExitIfFailed(
		device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)),
		L"B³¹d tworzenia kolejki komend!",
		COMMAND_QUEUE_CREATION_ERROR
	);
}

void Core::createCommandAllocator() {
	ErrorUtils::messageAndExitIfFailed(
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
		L"B³¹d tworzenia alokatora komend!",
		COMMAND_ALLOCATOR_CREATION_ERROR
	);
}

void Core::createCommandList() {
	ErrorUtils::messageAndExitIfFailed(
		device->CreateCommandList(
			NULL,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator.Get(),
			NULL, //Poniewa¿ na razie nie bêdziemy rysowaæ na ekranie
			IID_PPV_ARGS(&commandList)
		),
		L"B³¹d tworzenia alokatora komend!",
		COMMAND_ALLOCATOR_CREATION_ERROR
	);

	//Zamykamy, ¿eby zamkniêcie by³o stanem pocz¹tkowym
	//Dzieje siê tak dlatego, ¿e na pocz¹tku dodawania do list, bêdziemy wywo³ywaæ na niej Reset()
	//a ona wymaga by lista by³a zamkniêta.
	commandList->Close();
}

void Core::checkMultiSamplingSupport() {
	multiSamplingQualityLevels = {};
	multiSamplingQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	multiSamplingQualityLevels.Format = Core::pixelDefinitionFormat;
	multiSamplingQualityLevels.SampleCount = multiSamplingLevel; 
	multiSamplingQualityLevels.NumQualityLevels = 0; //Nie wiem czemu zero, sprawdziæ co to tak na prawdê znaczy

	ErrorUtils::messageAndExitIfFailed(
		device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&multiSamplingQualityLevels,
			sizeof(multiSamplingQualityLevels)
		),
		L"Twoja karta nie obs³uguje wymaganego multisamplingu! " + std::to_wstring(multiSamplingLevel) + L" :: Core::checkMultiSamplingSupport()",
		MULTISAMPLING_FEATURE_SUPPORT_ERROR
	);

	multiSamplingQualityLevel = multiSamplingQualityLevels.NumQualityLevels; //bêdzie 17, ale z docsów wynika ¿e mo¿na zmniejszyæ, ¿eby zwiêkszyæ wydajnoœæ

	ErrorUtils::messageAndExitIfFalse(
		multiSamplingQualityLevel > 0,
		L"Nieoczekiwana wartoœæ MSAA Quality Level! " + std::to_wstring(multiSamplingQualityLevel) + L" :: Core::checkMultiSamplingSupport()",
		UNEXPECTED_MSAA_QUALITY_LEVEL_ERROR
	);
}

void Core::defineDescriptorSizes() {
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); // bêdzie 32
	dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV); // bêdzie 8
	//SRV - shader render view
	//UAV - unordered render view
	cbvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // bêdzie 32
}

void Core::createFence() {
	HRESULT result = Core::device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&Core::fence)	
	);

	ErrorUtils::messageAndExitIfFailed(
		result,
		L"B³¹d przy tworzeniu blokady urz¹dzenia (Fence)! Core::createFence()",
		DEVICE_FENCE_CREATION_ERROR
	);
}

void Core::createFactory() {
	ErrorUtils::messageAndExitIfFailed(
		CreateDXGIFactory1(IID_PPV_ARGS(&Core::factory)),
		L"B³¹d przy tworzeniu fabryki! Core::createFactory()",
		FACTORY_CREATION_ERROR
	);
}

void Core::init() {
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ErrorUtils::messageAndExitIfFailed(
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)),
		L"B³¹d przejœcia w tryb debugowania! Core::init()",
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
}