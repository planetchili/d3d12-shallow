#include "App.h"
#include "GraphicsError.h"

#include <d3d12.h> 
#include <dxgi1_6.h> 
#include <DirectXMath.h> 
#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h" 
#pragma warning(pop)
#include <wrl.h>

#include <cmath> 
#include <numbers> 
#include <ranges>

namespace chil::app
{
	using Microsoft::WRL::ComPtr;
	using namespace DirectX;
	namespace rn = std::ranges;

	int Run(win::IWindow& window)
	{
		// constants 
		constexpr UINT width = 1280;
		constexpr UINT height = 720;
		constexpr UINT bufferCount = 2;

		// enable the software debug layer for d3d12 
		{
			ComPtr<ID3D12Debug> debugController;
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) >> chk;
			debugController->EnableDebugLayer();
		}

		// dxgi factory 
		ComPtr<IDXGIFactory4> dxgiFactory;
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)) >> chk;

		// device
		ComPtr<ID3D12Device2> device;
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)) >> chk;

		// command queue
		ComPtr<ID3D12CommandQueue> commandQueue;
		{
			const D3D12_COMMAND_QUEUE_DESC desc = {
				.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0,
			};
			device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)) >> chk;
		}

		// swap chain
		ComPtr<IDXGISwapChain4> swapChain;
		{
			const DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
				.Width = width,
				.Height = height,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.Stereo = FALSE,
				.SampleDesc = {
					.Count = 1,
					.Quality = 0
				},
				.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = bufferCount,
				.Scaling = DXGI_SCALING_STRETCH,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
				.Flags = 0,
			};
			ComPtr<IDXGISwapChain1> swapChain1;
			dxgiFactory->CreateSwapChainForHwnd(
				commandQueue.Get(),
				window.GetHandle(),
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1) >> chk;
			swapChain1.As(&swapChain) >> chk;
		}

		// rtv descriptor heap
		ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				.NumDescriptors = bufferCount,
			};
			device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescriptorHeap)) >> chk;
		}
		const auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// rtv descriptors and buffer references
		ComPtr<ID3D12Resource> backBuffers[bufferCount];
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
				rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			for (int i = 0; i < bufferCount; i++) {
				swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])) >> chk;
				device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
				rtvHandle.Offset(rtvDescriptorSize);
			}
		}

		// command allocator
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&commandAllocator)) >> chk;

		// command list
		ComPtr<ID3D12GraphicsCommandList> commandList;
		device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)) >> chk;
		// initially close the command list so it can be reset at top of draw loop 
		commandList->Close() >> chk;

		// fence
		ComPtr<ID3D12Fence> fence;
		uint64_t fenceValue = 0;
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) >> chk;

		// fence signalling event
		HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (!fenceEvent) {
			GetLastError() >> chk;
			throw std::runtime_error{ "Failed to create fence event" };
		}

		// data structure for vertex data 
		struct Vertex
		{
			XMFLOAT3 position;
			XMFLOAT3 color;
		};

		// create vertex buffer 
		ComPtr<ID3D12Resource> vertexBuffer;
		UINT nVertices;
		{
			// the content data 
			const Vertex vertexData[] = {
				{ {  0.00f,  0.50f, 0.0f }, { 1.0f, 0.0f, 0.0f } }, // top 
				{ {  0.43f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // right 
				{ { -0.43f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f } }, // left 
			};
			// set the vertex count 
			nVertices = (UINT)std::size(vertexData);
			// create committed resource for vertex buffer 
			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertexData));
				device->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr, IID_PPV_ARGS(&vertexBuffer)
				) >> chk;
			}
			// create committed resource for cpu upload of vertex data 
			ComPtr<ID3D12Resource> vertexUploadBuffer;
			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertexData));
				device->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr, IID_PPV_ARGS(&vertexUploadBuffer)
				) >> chk;
			}
			// copy array of vertex data to upload buffer 
			{
				Vertex* mappedVertexData = nullptr;
				vertexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
				rn::copy(vertexData, mappedVertexData);
				vertexUploadBuffer->Unmap(0, nullptr);
			}
			// reset command list and allocator  
			commandAllocator->Reset() >> chk;
			commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
			// copy upload buffer to vertex buffer 
			commandList->CopyResource(vertexBuffer.Get(), vertexUploadBuffer.Get());
			// close command list  
			commandList->Close() >> chk;
			// submit command list to queue as array with single element 
			ID3D12CommandList* const commandLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists((UINT)std::size(commandLists), commandLists);
			// insert fence to detect when upload is complete 
			commandQueue->Signal(fence.Get(), ++fenceValue) >> chk;
			fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
			if (WaitForSingleObject(fenceEvent, INFINITE) == WAIT_FAILED) {
				GetLastError() >> chk;
			}
		}

		// Create the vertex buffer view. 
		const D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
			.BufferLocation = vertexBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = nVertices * sizeof(Vertex),
			.StrideInBytes = sizeof(Vertex),
		};

		// render loop 
		UINT curBackBufferIndex;
		float t = 0.f;
		constexpr float step = 0.01f;
		while (!window.IsClosing()) {
			// advance back buffer
			curBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
			// select current buffer to render to 
			auto& backBuffer = backBuffers[curBackBufferIndex];
			// reset command list and allocator 
			commandAllocator->Reset() >> chk;
			commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
			// clear the render target 
			{
				// transition buffer resource to render target state 
				const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					backBuffer.Get(),
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				commandList->ResourceBarrier(1, &barrier);
				// calculate clear color 
				const FLOAT clearColor[] = {
					sin(2.f * t + 1.f) / 2.f + .5f,
					sin(3.f * t + 2.f) / 2.f + .5f,
					sin(5.f * t + 3.f) / 2.f + .5f,
					1.0f
				};
				// clear rtv 
				const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv{
					rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
					(INT)curBackBufferIndex, rtvDescriptorSize };
				commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
			}
			// prepare buffer for presentation by transitioning to present state
			{
				const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					backBuffer.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
				commandList->ResourceBarrier(1, &barrier);
			}
			// submit command list 
			{
				// close command list 
				commandList->Close() >> chk;
				// submit command list to queue as array with single element
				ID3D12CommandList* const commandLists[] = { commandList.Get() };
				commandQueue->ExecuteCommandLists((UINT)std::size(commandLists), commandLists);
			}
			// insert fence to mark command list completion 
			commandQueue->Signal(fence.Get(), ++fenceValue) >> chk;
			// present frame 
			swapChain->Present(1, 0) >> chk;
			// wait for command list / allocator to become free 
			fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
			if (WaitForSingleObject(fenceEvent, INFINITE) == WAIT_FAILED) {
				GetLastError() >> chk;
			}
			// update simulation time 
			if ((t += step) >= 2.f * std::numbers::pi_v<float>) {
				t = 0.f;
			}
		}

		// wait for queue to become completely empty (2 seconds max) 
		commandQueue->Signal(fence.Get(), ++fenceValue) >> chk;
		fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
		if (WaitForSingleObject(fenceEvent, 2000) == WAIT_FAILED) {
			GetLastError() >> chk;
		}

		return 0;
	}
}