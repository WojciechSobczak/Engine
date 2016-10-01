#pragma once
#include "stdafx.h"
#include <functional>

using namespace Microsoft::WRL; //For ComPtr


class Core {
public:
	typedef std::function<void()> RenderFunction;
	static void init(RenderFunction renderFunction);
	static void windowMainLoopHandler();
	static UINT getDisplayHeight() {
		return Core::displayHeight;
	};
	static UINT getDisplayWidth() {
		return Core::displayWidth;
	};
	
private:

	static HWND initializeMainWindow(HINSTANCE hInstance, std::wstring &windowName, bool showcmd = true);
	//LRESULT = long
	//CALLBACK = __stdcall
	static LRESULT CALLBACK messagesHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static RenderFunction mainRenderFunction;

	static ComPtr<ID3D12Device> device;
	static ComPtr<IDXGIFactory4> factory;
	static ComPtr<ID3D12Fence> fence;

	//RTV - render target view
	static UINT rtvDescriptorSize;
	//DSV - depth/stencil view
	static UINT dsvDescriptorSize;
	//CBV - constant buffer view
	static UINT cbvDescriptorSize;

	static D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multiSamplingQualityLevels;

	static DXGI_FORMAT pixelDefinitionFormat;
	static DXGI_FORMAT depthStencilFormat;

	static UINT multiSamplingLevel;
	static UINT multiSamplingQualityLevel;

	static ComPtr<ID3D12GraphicsCommandList> commandList;
	static ComPtr<ID3D12CommandQueue> commandQueue;
	static ComPtr<ID3D12CommandAllocator> commandAllocator;


	static UINT displayWidth;
	static UINT displayHeight;
	static UINT refreshRate;
	enum BufferingType {
		DOUBLE = 2,
		TRIPLE = 3
	};
	static BufferingType buffering;
	//static ComPtr<IDXGISwapChain> swapChain;
	static ComPtr<IDXGISwapChain1> swapChain;
	static ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	static ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	static ComPtr<ID3D12Resource> swapChainBackBuffers[];
	static ComPtr<ID3D12Resource> depthStencilBuffer;

	static INT currentBackBuffer;

	static HWND mainWindow;
	static std::wstring engineName;


	static D3D12_CPU_DESCRIPTOR_HANDLE getRTVHeapStartDescriptorHandle();
	static D3D12_CPU_DESCRIPTOR_HANDLE getDSVHeapStartDescriptorHandle();

	static void createWindow();
	static void calculateFrameStats();


	static void createDevice();
	static void createFactory();
	static void createFence();
	static void defineDescriptorSizes();
	static void checkMultiSamplingSupport();

	static void createCommandList();
	static void createCommandQueue();
	static void createCommandAllocator();

	static void createSwapChain();

	static void createRTVDescriptorHeap();
	static void createDSVDescriptorHeap();

	static void createSwapChainBuffersIntoRTVHeap();

	static void createDepthStencilView();

	static void createViewPort();
	static void createScissorsRectangle();

};
