/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "stdafx.h"

// Application Stuff
bool loop = true;

// Window Stuff (not Windows)
HWND hWnd;
int winWidth = 800;
int winHeight = 600;
bool fullscreen = false;

// D3D Stuff
const int frameBufferCount = 2; // Number of buffers to swap between
ID3D12Device* device; // Direct3D device
IDXGISwapChain3* swapChain; // Swapchain between render targets
ID3D12CommandQueue* commandQueue; // Container for command lists
ID3D12DescriptorHeap* rtvDescriptorHeap; // Descriptor heap to hold resources
ID3D12Resource* renderTargets[frameBufferCount]; // Number of render targets
ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // Enough allocators for each buffer * thread count
ID3D12GraphicsCommandList* commandList; // Records commands and execute to render frame
ID3D12Fence* fence[frameBufferCount];    // Locked while GPU runs commands, need as many as number of allocators
HANDLE fenceEvent; // Handle to event while GPU is running
UINT64 fenceValue[frameBufferCount]; // Value increments each frame
int frameIndex; // Current RTV
int rtvDescriptorSize; // Size of RTV Descriptor

// Function Declarations
bool InitD3D(); // Initializes Direct3D 12
void Update(); // Updates other logic
void UpdatePipeline(); // Updates command lists
void Render(); // Execute the command list
void CleanD3D(); // Release objects and clean memory
void WaitForPreviousFrame(); // Wait for GPU to finish command lists

int main(int argc, char* args[]) {
	// Create a window using SDL
	SDL_Window* window = SDL_CreateWindow(
		"DirectX 12 Purgatory", 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		800, 600, 0);
	hWnd = GetActiveWindow();

	// Initialize Direct3D
	if (!InitD3D()) {
		MessageBox(0, L"Failed to initialize direct3d 12",
			L"Error", MB_OK);
		CleanD3D();
		return 1;
	}

	// Loop application
	while (loop) {
		SDL_Event windowEvent;
		if (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) loop = false;
		}
		else {
			Update();
			Render();
		}
	}

	// Cleanup Direct3D
	WaitForPreviousFrame();
	CloseHandle(fenceEvent);
	CleanD3D();

	return 0;
}

bool InitD3D() {
	// Search for any adapters
	HRESULT hr;

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return false;
	}

	IDXGIAdapter1* adapter;
	int adapterIndex = 0;
	bool adapterFound = false;

	// Find adapter with DirectX 12 compatibility
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// Software adapter check
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			adapterIndex++;
			continue;
		}

		// Compatibility check
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr)) {
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	// Return false if valid adapter not found
	if (!adapterFound) {
		return false;
	}

	// Create the device using valid adapter
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
	if (FAILED(hr)) {
		return false;
	}

	// Creating the command queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(hr)) {
		return false;
	}

	// Display descriptor
	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = winWidth;
	backBufferDesc.Height = winHeight;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Sampling descriptor
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	// Describe the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = frameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !fullscreen;

	// Create the swap chain
	IDXGISwapChain* tempSwapChain;
	dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Describe an RTV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	if (FAILED(hr)) {
		return false;
	}

	// Get size of a descriptor
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a handle to first descriptor
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each buffer
	for (int i = 0; i < frameBufferCount; i++) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(hr)) {
			return false;
		}

		// "Create" a render target view
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

		// we increment the rtv handle by the rtv descriptor size we got above
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	// Create command allocators
	for (int i = 0; i < frameBufferCount; i++) {
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
		if (FAILED(hr)) {
			return false;
		}
	}

	// Create command list
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], NULL, IID_PPV_ARGS(&commandList));
	if (FAILED(hr)) {
		return false;
	}
	commandList->Close(); // Close for now, will be reopened in loop

	// Create fences
	for (int i = 0; i < frameBufferCount; i++) {
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
		if (FAILED(hr)) {
			return false;
		}
		fenceValue[i] = 0;
	}

	// Create handle to fence event
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		return false;
	}

	return true;
}

void Update() {
	// Update any application stuff
}

void UpdatePipeline() {
	HRESULT hr;

	// Wait for GPU to finish
	WaitForPreviousFrame();

	// Reset allocator when GPU is done
	hr = commandAllocator[frameIndex]->Reset();
	if (FAILED(hr)) {
		loop = false;
	}

	// Reset command lists
	hr = commandList->Reset(commandAllocator[frameIndex], NULL);
	if (FAILED(hr)) {
		loop = false;
	}

	// Reset render target to write
	CD3DX12_RESOURCE_BARRIER rb = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &rb);

	// Get handle to render target for merger stage
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	// Set render target to merger
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear render target to color
	const float clearColor[] = { 0.4f, 0.1f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Set render target to present state
	rb = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &rb);

	hr = commandList->Close();
	if (FAILED(hr)) {
		loop = false;
	}
}

void Render() {
	HRESULT hr;

	UpdatePipeline();

	// Create array of command lists
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// Execute command lists
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Last command in queue
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr)) {
		loop = false;
	}

	// Present the backbuffer
	hr = swapChain->Present(0, 0);
	if (FAILED(hr)) {
		loop = false;
	}
}

void CleanD3D() {
	// Wait for GPU to finish
	for (int i = 0; i < frameBufferCount; ++i)
	{
		frameIndex = i;
		WaitForPreviousFrame();
	}

	// Assert not fullscreen
	swapChain->SetFullscreenState(false, NULL);

	// Release objects
	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandList);

	for (int i = 0; i < frameBufferCount; ++i)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i]);
	};
}

void WaitForPreviousFrame() {
	HRESULT hr;

	// Swap to correct buffer
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Check if GPU has finished executing
	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		// Fence create an event
		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
		if (FAILED(hr)) {
			loop = false;
		}

		// Wait for event to finish
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// Increment for next frame
	fenceValue[frameIndex]++;
}
