#include "stdafx.h"
#include "DebugUtils.h"


void DebugUtils::listGraphicsCards(bool withMonitors /* = false */) {
	Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory(IID_PPV_ARGS(&factory));
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		wprintf(text.c_str());
		adapterList.push_back(adapter);

		++i;
	}

	if (withMonitors) {
		for (size_t i = 0; i < adapterList.size(); ++i) {
			IDXGIAdapter* adapter = adapterList[i];
			UINT j = 0;
			IDXGIOutput* output = nullptr;
			while (adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND) {
				DXGI_OUTPUT_DESC desc;
				output->GetDesc(&desc);

				std::wstring text = L"***Output: ";
				text += desc.DeviceName;
				text += L"\n";
				wprintf(text.c_str());
				++j;
			}
		}
	}
}


