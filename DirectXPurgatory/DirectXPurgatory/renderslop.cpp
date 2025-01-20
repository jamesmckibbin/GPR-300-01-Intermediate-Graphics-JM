#include "renderslop.h"

#include "app.h"

using namespace DirectX;

bool RenderSlop::Init(const HWND& window, bool screenState, float width, float height)
{
	HRESULT result;

	// Find Compatible Adapter
	IDXGIAdapter1* adapter = {};
	if (!FindCompatibleAdapter(adapter)) {
		return false;
	}

	// Create Device
	result = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
	if (FAILED(result)) {
		return false;
	}

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	result = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(result)) {
		return false;
	}

	// Create Sample Descriptor
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	// Create Swap Chain
	CreateSwapChain(window, sampleDesc, screenState);

	// Create RTV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	if (FAILED(result)) {
		return false;
	}

	// Get Size & First Handle of RTV Descriptor
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create RTV Per Buffer
	for (int i = 0; i < frameBufferCount; i++) {
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(result)) {
			return false;
		}
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	// Create Command Allocators & Command List
	CreateCommandList();

	// Create Fence & Fence Event Handle
	CreateFence();

	// Create Constant Buffer Descriptor Table Range
	D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1];
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorTableRanges[0].NumDescriptors = 1;
	descriptorTableRanges[0].BaseShaderRegister = 0;
	descriptorTableRanges[0].RegisterSpace = 0;
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Create Root Descriptor
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// Create Root Parameters
	D3D12_ROOT_PARAMETER  rootParameters[1];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor = rootCBVDescriptor;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Create Root Signature Descriptor
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters),
		rootParameters,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

	// Create Root Signature
	ID3DBlob* signature;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	result = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(result))
	{
		return false;
	}

	// Create Shaders
	Shader vertexShader;
	vertexShader.Init(L"VertexShader.hlsl", "main", "vs_5_0");

	Shader pixelShader;
	pixelShader.Init(L"PixelShader.hlsl", "main", "ps_5_0");

	// Create Input Layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Fill Input Layout Descriptor
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// Create PSO Descriptor
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS = vertexShader.GetBytecode();
	psoDesc.PS = pixelShader.GetBytecode();
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	// Create PSO
	result = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(result))
	{
		return false;
	}

	// Verticies
	Vertex vList[] = {
		// Front
		{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

		// Right
		{  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

		// Left
		{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

		// Back
		{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

		// Top
		{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },

		// Bottom
		{  0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	};
	int vBufferSize = sizeof(vList);

	// Verticies Default Heap
	CD3DX12_HEAP_PROPERTIES dHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(vBufferSize);
	device->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer));
	vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// Verticies Upload Heap
	CD3DX12_HEAP_PROPERTIES uHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ID3D12Resource* vBufferUploadHeap;
	device->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));
	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// Copy Verticies to Default Heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList);
	vertexData.RowPitch = vBufferSize;
	vertexData.SlicePitch = vBufferSize;
	UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &resoBarr);

	// Indicies
	DWORD iList[] = {
		// Front
		0, 1, 2,
		0, 3, 1,

		// Left
		4, 5, 6,
		4, 7, 5,

		// Right
		8, 9, 10,
		8, 11, 9,

		// Back
		12, 13, 14,
		12, 15, 13,

		// Top
		16, 17, 18,
		16, 19, 17,

		// Bottom
		20, 21, 22,
		20, 23, 21,
	};
	int iBufferSize = sizeof(iList);
	numCubeIndices = sizeof(iList) / sizeof(DWORD);

	// Indicies Default Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(iBufferSize);
	device->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&indexBuffer));
	indexBuffer->SetName(L"Index Buffer Resource Heap");

	// Indicies Upload Heap
	ID3D12Resource* iBufferUploadHeap;
	device->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));
	iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// Copy Indicies to Default Heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList);
	indexData.RowPitch = iBufferSize;
	indexData.SlicePitch = iBufferSize;
	UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	commandList->ResourceBarrier(1, &resoBarr);

	// Create Depth Stencil Buffer Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
	if (FAILED(result))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	resoDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, (UINT64)width, (UINT)height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	device->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer));
	dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create Constant Buffer Resource Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
	for (int i = 0; i < frameBufferCount; ++i)
	{
		result = device->CreateCommittedResource(
			&uHeapProp,
			D3D12_HEAP_FLAG_NONE,
			&resoDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constantBufferUploadHeaps[i]));
		constantBufferUploadHeaps[i]->SetName(L"Constant Buffer Upload Resource Heap");

		ZeroMemory(&cbPerObject, sizeof(cbPerObject));

		CD3DX12_RANGE readRange(0, 0);

		result = constantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbvGPUAddress[i]));

		memcpy(cbvGPUAddress[i], &cbPerObject, sizeof(cbPerObject)); // Cube 1
		memcpy(cbvGPUAddress[i] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject)); // Cube 2
	}

	// Execute Command List
	commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Increment Fence Value
	fenceValue[frameIndex]++;
	result = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(result))
	{
		return false;
	}

	// Create VBV
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vBufferSize;

	// Create Index Buffer View
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = iBufferSize;

	// Define Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Define Scissor Rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = (LONG)width;
	scissorRect.bottom = (LONG)height;

	// build projection and view matrix
	DirectX::XMMATRIX tmpMat = DirectX::XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)width / (float)height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&cameraProjMat, tmpMat);

	// set starting camera state
	cameraPosition = DirectX::XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
	cameraTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	cameraUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// build view matrix
	DirectX::XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
	DirectX::XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
	DirectX::XMVECTOR cUp = XMLoadFloat4(&cameraUp);
	tmpMat = DirectX::XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&cameraViewMat, tmpMat);

	// set starting cubes position
	// first cube
	cube1Position = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR posVec = XMLoadFloat4(&cube1Position);

	tmpMat = DirectX::XMMatrixTranslationFromVector(posVec);
	XMStoreFloat4x4(&cube1RotMat, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&cube1WorldMat, tmpMat);

	// second cube
	cube2PositionOffset = DirectX::XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
	posVec = DirectX::XMLoadFloat4(&cube2PositionOffset) + DirectX::XMLoadFloat4(&cube1Position);

	tmpMat = DirectX::XMMatrixTranslationFromVector(posVec);
	XMStoreFloat4x4(&cube2RotMat, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&cube2WorldMat, tmpMat);

	return true;
}

void RenderSlop::UnInit()
{
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
		SAFE_RELEASE(constantBufferUploadHeaps[i]);
	};
}

void RenderSlop::Update()
{
	// create rotation matrices
	DirectX::XMMATRIX rotXMat = DirectX::XMMatrixRotationX(0.0001f);
	DirectX::XMMATRIX rotYMat = DirectX::XMMatrixRotationY(0.0002f);
	DirectX::XMMATRIX rotZMat = DirectX::XMMatrixRotationZ(0.0003f);

	// add rotation to cube1's rotation matrix and store it
	DirectX::XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotXMat * rotYMat * rotZMat;
	XMStoreFloat4x4(&cube1RotMat, rotMat);

	// create translation matrix for cube 1 from cube 1's position vector
	DirectX::XMMATRIX translationMat = DirectX::XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

	// create cube1's world matrix by first rotating the cube, then positioning the rotated cube
	DirectX::XMMATRIX worldMat = rotMat * translationMat;

	// store cube1's world matrix
	XMStoreFloat4x4(&cube1WorldMat, worldMat);

	// update constant buffer for cube1
	// create the wvp matrix and store in constant buffer
	DirectX::XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat); // load view matrix
	DirectX::XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat); // load projection matrix
	DirectX::XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat; // create wvp matrix
	DirectX::XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(cbvGPUAddress[frameIndex], &cbPerObject, sizeof(cbPerObject));

	// now do cube2's world matrix
	// create rotation matrices for cube2
	rotXMat = DirectX::XMMatrixRotationX(0.0003f);
	rotYMat = DirectX::XMMatrixRotationY(0.0002f);
	rotZMat = DirectX::XMMatrixRotationZ(0.0001f);

	// add rotation to cube2's rotation matrix and store it
	rotMat = rotZMat * (XMLoadFloat4x4(&cube2RotMat) * (rotXMat * rotYMat));
	XMStoreFloat4x4(&cube2RotMat, rotMat);

	// create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
	DirectX::XMMATRIX translationOffsetMat = DirectX::XMMatrixTranslationFromVector(XMLoadFloat4(&cube2PositionOffset));

	// we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
	DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);

	// reuse worldMat. 
	// first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
	// then we translate it. 
	// then we rotate it. rotation always rotates around point 0,0,0
	// finally we move it to cube 1's position, which will cause it to rotate around cube 1
	worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

	wvpMat = XMLoadFloat4x4(&cube2WorldMat) * viewMat * projMat; // create wvp matrix
	transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(cbvGPUAddress[frameIndex] + ConstantBufferPerObjectAlignedSize, &cbPerObject, sizeof(cbPerObject));

	// store cube2's world matrix
	XMStoreFloat4x4(&cube2WorldMat, worldMat);
}

void RenderSlop::UpdatePipeline()
{
	HRESULT result;

	// Wait for GPU to finish
	WaitForPreviousFrame();

	// Reset allocator when GPU is done
	result = commandAllocator[frameIndex]->Reset();
	if (FAILED(result)) {
		running = false;
	}

	// Reset command lists
	result = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
	if (FAILED(result)) {
		running = false;
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
	const float clearColor[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// PUT RENDER COMMANDS HERE
	commandList->SetGraphicsRootSignature(rootSignature);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);
	
	commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress());
	commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

	commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[frameIndex]->GetGPUVirtualAddress() + ConstantBufferPerObjectAlignedSize);
	commandList->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

	// Set render target to present state
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &resoBarr);

	result = commandList->Close();
	if (FAILED(result)) {
		running = false;
	}
}

void RenderSlop::Render()
{
	HRESULT result;

	UpdatePipeline();

	// Create array of command lists
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// Execute command lists
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Last command in queue
	result = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(result)) {
		running = false;
	}

	// Present the backbuffer
	result = swapChain->Present(0, 0);
	if (FAILED(result)) {
		running = false;
	}
}

void RenderSlop::WaitForPreviousFrame()
{
	HRESULT result;

	// Swap to correct buffer
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Check if GPU has finished executing
	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		// Fence create an event
		result = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
		if (FAILED(result)) {
			running = false;
		}

		// Wait for event to finish
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// Increment for next frame
	fenceValue[frameIndex]++;
}

void RenderSlop::CloseFenceEventHandle() { CloseHandle(fenceEvent); }

bool RenderSlop::FindCompatibleAdapter(IDXGIAdapter1* adap)
{
	HRESULT result;

	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(result)) {
		return false;
	}

	int adapterIndex = 0;
	bool adapterFound = false;

	// Find adapter with DirectX 12 compatibility
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adap) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adap->GetDesc1(&desc);

		// Software adapter check
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			adapterIndex++;
			continue;
		}

		// Compatibility check
		result = D3D12CreateDevice(adap, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(result)) {
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	return true;
}

void RenderSlop::CreateSwapChain(const HWND& window, DXGI_SAMPLE_DESC sampleDesc, bool screenState)
{
	// Display descriptor
	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = DEFAULT_WINDOW_WIDTH;
	backBufferDesc.Height = DEFAULT_WINDOW_HEIGHT;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Describe the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = frameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = window;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !screenState;

	// Create the swap chain
	IDXGISwapChain* tempSwapChain;
	dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

bool RenderSlop::CreateCommandList()
{
	HRESULT result;
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

	return true;
}

bool RenderSlop::CreateFence()
{
	HRESULT result;

	for (int i = 0; i < frameBufferCount; i++) {
		result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
		if (FAILED(result)) {
			return false;
		}
		fenceValue[i] = 0;
	}

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		return false;
	}

	return true;
}

bool Shader::Init(LPCWSTR filename, LPCSTR entryFunc, LPCSTR target)
{
	HRESULT result;

	result = D3DCompileFromFile(filename, nullptr, nullptr,
		entryFunc, target, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &shaderBlob, &errorBlob);
	if (FAILED(result))
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return false;
	}

	shaderBytecode = {};
	shaderBytecode.BytecodeLength = shaderBlob->GetBufferSize();
	shaderBytecode.pShaderBytecode = shaderBlob->GetBufferPointer();

	return true;
}

ID3DBlob* Shader::GetBlob() { return shaderBlob; }

ID3DBlob* Shader::GetErrorBlob() { return errorBlob; }

D3D12_SHADER_BYTECODE Shader::GetBytecode() { return shaderBytecode; }
