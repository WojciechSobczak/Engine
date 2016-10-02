#include "stdafx.h"
#include "ErrorUtils.h"

void Renderer::drawSomething() {

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished
	// execution on the GPU.
	ThrowIfFailed(Core::commandAllocator->Reset());

	// A command list can be reset after it has been added to the 
	// command queue via ExecuteCommandList. Reusing the command list reuses memory.
	ThrowIfFailed(Core::commandList->Reset(Core::commandAllocator.Get(), NULL));

	// Set the viewport and scissor rect. This needs to be reset 
	// whenever the command list is reset.
	Core::commandList->RSSetViewports(1, &Core::viewport);
	Core::commandList->RSSetScissorRects(1, &Core::scissorsRectangle);

	// Indicate a state transition on the resource usage.
	Core::commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			Core::getCurrentBackBuffer().Get(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	);



	// Specify the buffers we are going to render to.
	Core::commandList->OMSetRenderTargets(
		1,
		&Core::getCurrentBackBufferView(),
		true,
		&Core::getDSVHeapStartDescriptorHandle()
	);

	// Clear the back buffer and depth buffer.
	Core::commandList->ClearRenderTargetView(
		Core::getCurrentBackBufferView(),
		DirectX::Colors::LightSteelBlue,
		0,
		NULL
	);

	Core::commandList->ClearDepthStencilView(
		Core::getDSVHeapStartDescriptorHandle(),
		D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		NULL
	);


	//// Indicate a state transition on the resource usage.
	Core::commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			Core::getCurrentBackBuffer().Get(),
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT
		)
	);

	// Done recording commands.
	ThrowIfFailed(Core::commandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = {Core::commandList.Get()};
	Core::commandQueue->ExecuteCommandLists(1, cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(Core::swapChain->Present(0, 0));
	Core::currentBackBuffer = (Core::currentBackBuffer + 1) % Core::buffering;
	Core::flushCommandQueue();
}