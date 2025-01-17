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

// Shader Stuff
ID3D12PipelineState* pipelineStateObject; // Contains pipeline state
ID3D12RootSignature* rootSignature; // Defines the data that shaders will access
D3D12_VIEWPORT viewport; // Output area
D3D12_RECT scissorRect; // The area to draw in
ID3D12Resource* vertexBuffer; // Buffer in GPU to load vertex data into
D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // Pointer to vertex data in GPU

// Vertex Definition
struct Vertex {
	DirectX::XMFLOAT3 pos;
};

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
	HRESULT result;

	IDXGIFactory4* dxgiFactory;
	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(result)) {
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
		result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(result)) {
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
	result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
	if (FAILED(result)) {
		return false;
	}

	// Creating the command queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	result = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(result)) {
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
	result = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	if (FAILED(result)) {
		return false;
	}

	// Get size of a descriptor
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a handle to first descriptor
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each buffer
	for (int i = 0; i < frameBufferCount; i++) {
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(result)) {
			return false;
		}

		// "Create" a render target view
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

		// we increment the rtv handle by the rtv descriptor size we got above
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	// Create command allocators
	for (int i = 0; i < frameBufferCount; i++) {
		result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
		if (FAILED(result)) {
			return false;
		}
	}

	// Create command list
	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], NULL, IID_PPV_ARGS(&commandList));
	if (FAILED(result)) {
		return false;
	}
	commandList->Close(); // Close for now, will be reopened in loop

	// Create fences
	for (int i = 0; i < frameBufferCount; i++) {
		result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
		if (FAILED(result)) {
			return false;
		}
		fenceValue[i] = 0;
	}

	// Create handle to fence event
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		return false;
	}

	// Init root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Grab root signature bytecode
	ID3DBlob* signature;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	// Create root signature
	result = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(result))
	{
		return false;
	}

	// Compile vertex shader
	ID3DBlob* vertexShader;
	ID3DBlob* errorBuff; // Error data buffer
	result = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr,
		"main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vertexShader, &errorBuff);
	if (FAILED(result))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// Fill out a shader bytecode structure for vertex shader
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
	
	// Compile pixel shader
	ID3DBlob* pixelShader;
	result = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr,
		"main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &pixelShader, &errorBuff);
	if (FAILED(result))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// Fill out a shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	// Create input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Fill out input layout descriptor
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// Define the pipeline state object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // PSO descriptor
	psoDesc.InputLayout = inputLayoutDesc; // Describes out input layout
	psoDesc.pRootSignature = rootSignature; // The root signature associated with this PSO
	psoDesc.VS = vertexShaderBytecode; // Vertex shader bytcode
	psoDesc.PS = pixelShaderBytecode; // Pixel shader bytecode
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Topology type
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Format of render target
	psoDesc.SampleDesc = sampleDesc; // Must be same sample description as swapchain and depth buffer
	psoDesc.SampleMask = 0xffffffff; // Multi-sampling
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // Default rasterizer state
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // Default blend state
	psoDesc.NumRenderTargets = 1;

	// Create the pipeline state object
	result = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(result))
	{
		return false;
	}

	// Le triangle
	Vertex vList[] = {
		{ { 0.0f, 0.5f, 0.5f } },
		{ { -0.5f, -0.5f, 0.5f } },
		{ { 0.5f, -0.5f, 0.5f } },
	};
	int vBufferSize = sizeof(vList);

	// Create default heap
	CD3DX12_HEAP_PROPERTIES dHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
	device->CreateCommittedResource(
		&dHeapProp, // Default heap
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, // Copy data from upload heap to this heap
		nullptr,
		IID_PPV_ARGS(&vertexBuffer));

	// Name the buffer
	vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// Create upload heap
	CD3DX12_HEAP_PROPERTIES uHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ID3D12Resource* vBufferUploadHeap;
	device->CreateCommittedResource(
		&uHeapProp, // Upload heap
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU reads from this buffer
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));

	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// Store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList); // Pointer to vertex array
	vertexData.RowPitch = vBufferSize; // Size of vertex array data
	vertexData.SlicePitch = vBufferSize; // ^

	// Command to copy upload heap to default heap
	UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

	// Transition from copy state to vertex buffer state
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &resoBarr);

	// Execute command list to upload triangle data
	commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Increment the fence value so buffer will load
	fenceValue[frameIndex]++;
	result = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(result))
	{
		loop = false;
	}

	// Create VBV for the triangle
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vBufferSize;

	// Fill out the Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = winWidth;
	viewport.Height = winHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = winWidth;
	scissorRect.bottom = winHeight;

	return true;
}

void Update() {
	// Update any application stuff
}

void UpdatePipeline() {
	HRESULT result;

	// Wait for GPU to finish
	WaitForPreviousFrame();

	// Reset allocator when GPU is done
	result = commandAllocator[frameIndex]->Reset();
	if (FAILED(result)) {
		loop = false;
	}

	// Reset command lists
	result = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
	if (FAILED(result)) {
		loop = false;
	}

	CD3DX12_RESOURCE_BARRIER resoBarr;

	// Reset render target to write
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &resoBarr);

	// Get handle to render target for merger stage
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);

	// Set render target to merger
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear render target to color
	const float clearColor[] = { 0.4f, 0.1f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// PUT RENDER COMMANDS HERE
	commandList->SetGraphicsRootSignature(rootSignature); // Set root signature
	commandList->RSSetViewports(1, &viewport); // Set viewports
	commandList->RSSetScissorRects(1, &scissorRect); // Set scissor rects
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Set primitive topology
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // Set vertex buffer
	commandList->DrawInstanced(3, 1, 0, 0); // Draw 3 vertices

	// Set render target to present state
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &resoBarr);

	result = commandList->Close();
	if (FAILED(result)) {
		loop = false;
	}
}

void Render() {
	HRESULT result;

	UpdatePipeline();

	// Create array of command lists
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// Execute command lists
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Last command in queue
	result = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(result)) {
		loop = false;
	}

	// Present the backbuffer
	result = swapChain->Present(0, 0);
	if (FAILED(result)) {
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

	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(vertexBuffer);
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
