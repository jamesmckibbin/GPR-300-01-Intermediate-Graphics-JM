/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "stdafx.h"

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

bool InitD3D(); // Initializes Direct3D 12
void Frame(); // Updates other logic
void UpdatePipeline(); // Updates command lists
void Render(); // Execute the command list
void Cleanup(); // Release objects and clean memory
void WaitForPreviousFrame(); // Wait for GPU to finish command lists

typedef struct D3D12_COMMAND_QUEUE_DESC {
	D3D12_COMMAND_LIST_TYPE   Type; // Direct, Compute, and Copy | Direct: All | Compute: Compute & Copy | Copy: Just Copy
	INT                       Priority; // Used if multiple queues are present and one needs priority
	D3D12_COMMAND_QUEUE_FLAGS Flags; // Basically, dont touch
	UINT                      NodeMask; // Used only for multiple GPUs
} D3D12_COMMAND_QUEUE_DESC;

typedef struct D3D12_DESCRIPTOR_HEAP_DESC {
	D3D12_DESCRIPTOR_HEAP_TYPE  Type; // CBV/SRV/UAV, Sampler, RTV, and DSV
	UINT                        NumDescriptors; // Number of descriptors/buffers
	D3D12_DESCRIPTOR_HEAP_FLAGS Flags; // Should be shader visible or not
	UINT                        NodeMask; // Bitfield that determines the GPU this heap is stored on
} D3D12_DESCRIPTOR_HEAP_DESC;

typedef struct DXGI_MODE_DESC {
	UINT                     Width; // Width of the backbuffer
	UINT                     Height; // Height of the backbuffer
	DXGI_RATIONAL            RefreshRate; // Defines refresh rate of swapchain
	DXGI_FORMAT              Format; // Display format of swapchain
	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering; // Scanline drawing mode
	DXGI_MODE_SCALING        Scaling; // Defines if buffer is centered or should be stretched
} DXGI_MODE_DESC;

typedef struct DXGI_SAMPLE_DESC {
	UINT Count; // Number of samples at each pixel
	UINT Quality; // Quality of samples taken
} DXGI_SAMPLE_DESC;

typedef struct DXGI_SWAP_CHAIN_DESC {
	DXGI_MODE_DESC   BufferDesc; // Describes display mode info
	DXGI_SAMPLE_DESC SampleDesc; // Describes multi-sampling
	DXGI_USAGE       BufferUsage; // Render target or shader input
	UINT             BufferCount; // Number of back buffers
	HWND             OutputWindow; // Handle to the window
	BOOL             Windowed; // Fullscreen or windowed
	DXGI_SWAP_EFFECT SwapEffect; // What happens to buffers after presented
	UINT             Flags; // Any flags
} DXGI_SWAP_CHAIN_DESC;

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
		Cleanup();
		return 1;
	}

	// Loop application
	while (true) {
		SDL_Event windowEvent;
		if (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) break;
		}
		else {
			Frame();
		}
	}

	// Cleanup Direct3D
	WaitForPreviousFrame();
	CloseHandle(fenceEvent);

	return 0;
}

bool InitD3D()
{
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
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
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
}
