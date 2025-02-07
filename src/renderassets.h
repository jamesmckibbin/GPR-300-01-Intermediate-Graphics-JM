#pragma once

#include "gconst.h"

class RenderAssets {

public:

	bool Init(const HWND& window, bool screenState);
	void UnInit();

	ID3D12Device* GetDevice() { return device; }
	IDXGISwapChain3* GetSwapChain() { return swapChain; }
	ID3D12CommandQueue* GetCommandQueue() { return commandQueue; }
	ID3D12CommandAllocator* GetCommandAllocator(int index) { return commandAllocator[index]; }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList; }
	ID3D12Fence* GetFence(int index) { return fence[index]; }
	ID3D12Fence* IncrementFence(int index) { return fence[index]++; }
	HANDLE GetFenceEvent() { return fenceEvent; }
	UINT64 GetFenceValue(int index) { return fenceValue[index]; }
	UINT64 IncrementFenceValue(int index) { return fenceValue[index]++; }
	int GetFrameIndex() { return frameIndex; }
	void SetFrameIndex(int i) { frameIndex = i; }
	ID3D12DescriptorHeap* GetRtvDescriptorHeap() { return rtvDescriptorHeap; }
	ID3D12Resource* GetRenderTarget(int index) { return renderTargets[index]; }
	int GetRtvDescriptorSize() { return rtvDescriptorSize; }
	
private:

	bool FindCompatibleAdapter(IDXGIAdapter1* adap);
	void CreateSwapChain(const HWND& window, DXGI_SAMPLE_DESC sampleDesc, bool screenState);
	bool CreateCommandList();
	bool CreateFence();

	// Device & Swapchain
	ID3D12Device* device;
	IDXGIFactory4* dxgiFactory;
	IDXGISwapChain3* swapChain;

	// Commands
	ID3D12CommandQueue* commandQueue;
	ID3D12CommandAllocator* commandAllocator[FRAME_BUFFER_COUNT];
	ID3D12GraphicsCommandList* commandList;
	ID3D12Fence* fence[FRAME_BUFFER_COUNT];
	HANDLE fenceEvent;
	UINT64 fenceValue[FRAME_BUFFER_COUNT];

	// Render Target View
	ID3D12DescriptorHeap* rtvDescriptorHeap;
	ID3D12Resource* renderTargets[FRAME_BUFFER_COUNT];
	int frameIndex;
	int rtvDescriptorSize;

};