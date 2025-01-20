/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "stdafx.h"

// Vertex Definition
struct Vertex {
	Vertex(float x, float y, float z, float r, float g, float b, float a) : pos(x, y, z), color(r, g, b, a) {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
};

// Constant Buffer Definition
struct ConstantBuffer {
	DirectX::XMFLOAT4 colorMultiplier;
};

// Application Stuff
bool loop = true;

// Window Stuff (not Windows)
HWND hWnd;

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

ID3D12Resource* indexBuffer; // a default buffer in GPU memory that we will load index data for our triangle into
D3D12_INDEX_BUFFER_VIEW indexBufferView; // a structure holding information about the index buffer

ID3D12Resource* depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
ID3D12DescriptorHeap* dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

ID3D12DescriptorHeap* mainDescriptorHeap[frameBufferCount]; // this heap will store the descripor to our constant buffer
ID3D12Resource* constantBufferUploadHeap[frameBufferCount]; // this is the memory on the gpu where our constant buffer will be placed.
ConstantBuffer cbColorMultiplierData; // this is the constant buffer data we will send to the gpu 
UINT8* cbColorMultiplierGPUAddress[frameBufferCount]; // this is a pointer to the memory location we get when we map our constant buffer

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
		DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0);
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
	backBufferDesc.Width = DEFAULT_WINDOW_WIDTH;
	backBufferDesc.Height = DEFAULT_WINDOW_HEIGHT;
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
	swapChainDesc.Windowed = !fullScreen;

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

	// create a descriptor range (descriptor table) and fill it out
	// this is a range of descriptors inside a descriptor heap
	D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1]; // only one range right now
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // this is a range of constant buffer views (descriptors)
	descriptorTableRanges[0].NumDescriptors = 1; // we only have one constant buffer, so the range is only 1
	descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

	// create a descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

	D3D12_ROOT_PARAMETER  rootParameters[1]; // only one parameter right now
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
	rootParameters[0].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	// Init root signature
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

	// Create the pipeline state object
	result = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(result))
	{
		return false;
	}

	// Le triangle
	Vertex vList[] = {
		// First quad
		{ -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
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

	// Le indicies
	DWORD iList[] = {
		0, 1, 2,
		0, 3, 1
	};
	int iBufferSize = sizeof(iList);

	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);
	// create default heap to hold index buffer
	device->CreateCommittedResource(
		&dHeapProp, // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&resoDesc, // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
		nullptr, // optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&indexBuffer));

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	vertexBuffer->SetName(L"Index Buffer Resource Heap");

	// create upload heap to upload index buffer
	ID3D12Resource* iBufferUploadHeap;
	device->CreateCommittedResource(
		&uHeapProp, // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&resoDesc, // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));
	vBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
	indexData.RowPitch = iBufferSize; // size of all our index buffer
	indexData.SlicePitch = iBufferSize; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &resoBarr);

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	indexBufferView.SizeInBytes = iBufferSize;

	// Create the depth/stencil buffer

	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
	if (FAILED(result))
	{
		loop = false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	resoDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, currentWindowWidth, currentWindowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	device->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer)
	);
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
	if (FAILED(result))
	{
		loop = false;
	}
	dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a constant buffer descriptor heap for each frame
	// this is the descriptor heap that will store our constant buffer descriptor
	for (int i = 0; i < frameBufferCount; ++i)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		result = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap[i]));
		if (FAILED(result))
		{
			loop = false;
		}
	}

	// create the constant buffer resource heap
	// We will update the constant buffer one or more times per frame, so we will use only an upload heap
	// unlike previously we used an upload heap to upload the vertex and index data, and then copied over
	// to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
	// efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
	// will be modified and uploaded at least once per frame, so we only use an upload heap

	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);

	// create a resource heap, descriptor heap, and pointer to cbv for each frame
	for (int i = 0; i < frameBufferCount; ++i)
	{
		result = device->CreateCommittedResource(
			&uHeapProp, // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&resoDesc, // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&constantBufferUploadHeap[i]));
		constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constantBufferUploadHeap[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;    // CB size is required to be 256-byte aligned.
		device->CreateConstantBufferView(&cbvDesc, mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());

		ZeroMemory(&cbColorMultiplierData, sizeof(cbColorMultiplierData));

		CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (End is less than or equal to begin)
		result = constantBufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbColorMultiplierGPUAddress[i]));
		memcpy(cbColorMultiplierGPUAddress[i], &cbColorMultiplierData, sizeof(cbColorMultiplierData));
	}

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
	viewport.Width = currentWindowWidth;
	viewport.Height = currentWindowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = currentWindowWidth;
	scissorRect.bottom = currentWindowHeight;

	return true;
}

void Update() {
	// update app logic, such as moving the camera or figuring out what objects are in view
	static float rIncrement = 0.00002f;
	static float gIncrement = 0.00006f;
	static float bIncrement = 0.00009f;

	cbColorMultiplierData.colorMultiplier.x += rIncrement;
	cbColorMultiplierData.colorMultiplier.y += gIncrement;
	cbColorMultiplierData.colorMultiplier.z += bIncrement;

	if (cbColorMultiplierData.colorMultiplier.x >= 1.0 || cbColorMultiplierData.colorMultiplier.x <= 0.0)
	{
		cbColorMultiplierData.colorMultiplier.x = cbColorMultiplierData.colorMultiplier.x >= 1.0 ? 1.0 : 0.0;
		rIncrement = -rIncrement;
	}
	if (cbColorMultiplierData.colorMultiplier.y >= 1.0 || cbColorMultiplierData.colorMultiplier.y <= 0.0)
	{
		cbColorMultiplierData.colorMultiplier.y = cbColorMultiplierData.colorMultiplier.y >= 1.0 ? 1.0 : 0.0;
		gIncrement = -gIncrement;
	}
	if (cbColorMultiplierData.colorMultiplier.z >= 1.0 || cbColorMultiplierData.colorMultiplier.z <= 0.0)
	{
		cbColorMultiplierData.colorMultiplier.z = cbColorMultiplierData.colorMultiplier.z >= 1.0 ? 1.0 : 0.0;
		bIncrement = -bIncrement;
	}

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(cbColorMultiplierGPUAddress[frameIndex], &cbColorMultiplierData, sizeof(cbColorMultiplierData));
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

	// Get handle to render target and depth buffer for merger stage
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Set render target to merger
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear render target and depth buffer
	const float clearColor[] = { 0.4f, 0.1f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// PUT RENDER COMMANDS HERE
	ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap[frameIndex] };
	commandList->SetGraphicsRootSignature(rootSignature); // Set root signature
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	commandList->SetGraphicsRootDescriptorTable(0, mainDescriptorHeap[frameIndex]->GetGPUDescriptorHandleForHeapStart());
	commandList->RSSetViewports(1, &viewport); // Set viewports
	commandList->RSSetScissorRects(1, &scissorRect); // Set scissor rects
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Set primitive topology
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // Set vertex buffer
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	commandList->DrawIndexedInstanced(6, 1, 0, 4, 0);

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
	SAFE_RELEASE(indexBuffer);

	SAFE_RELEASE(depthStencilBuffer);
	SAFE_RELEASE(dsDescriptorHeap);

	for (int i = 0; i < frameBufferCount; ++i)
	{
		SAFE_RELEASE(mainDescriptorHeap[i]);
		SAFE_RELEASE(constantBufferUploadHeap[i]);
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
