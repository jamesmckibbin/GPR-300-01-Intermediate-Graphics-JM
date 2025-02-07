#include "renderer.h"

#include "app.h"

using namespace DirectX;

DescriptorHeapAllocator Renderer::fontDescriptorHeapAlloc = {};

bool Renderer::Init(const HWND& window, bool screenState, float width, float height)
{
	HRESULT result;

	// Create Render Assets
	assets = new RenderAssets();
	if (!assets->Init(window, screenState))
	{
		return false;
	}

	textureManager = new TextureManager();

	// Create Sample Descriptor
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	// Create Root Descriptor
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// Create Descriptor Range
	D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1];
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorTableRanges[0].NumDescriptors = 1;
	descriptorTableRanges[0].BaseShaderRegister = 0;
	descriptorTableRanges[0].RegisterSpace = 0;
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Create Descriptor Table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

	// Create CBV Constant Buffer Root Paramater
	D3D12_ROOT_PARAMETER  rootParameters[2];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor = rootCBVDescriptor;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Create SRV Texture Root Parameter
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable = descriptorTable;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create Static Sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Create Root Signature Descriptor
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters),
		rootParameters,
		1,
		&sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	// Create Root Signature
	ID3DBlob* signature;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(result))
	{
		return false;
	}

	result = assets->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(result))
	{
		return false;
	}

	// Create Shaders
	Shader vertexShader{};
	vertexShader.Init(L"shaders/VertexShader.hlsl", "main", "vs_5_0");

	Shader pixelShader{};
	pixelShader.Init(L"shaders/PixelShader.hlsl", "main", "ps_5_0");

	// Create PP Shaders
	Shader ppVertexShader{};
	ppVertexShader.Init(L"shaders/PPVertexShader.hlsl", "main", "vs_5_0");

	Shader ppPixelShader{};
	ppPixelShader.Init(L"shaders/PPPixelShader.hlsl", "main", "ps_5_0");

	// Create Input Layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMALS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
	result = assets->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(result))
	{
		return false;
	}

	// Create Framebuffer PSO Descriptor
	D3D12_GRAPHICS_PIPELINE_STATE_DESC fbPsoDesc = {};
	fbPsoDesc.InputLayout = inputLayoutDesc;
	fbPsoDesc.pRootSignature = rootSignature;
	fbPsoDesc.VS = ppVertexShader.GetBytecode();
	fbPsoDesc.PS = ppPixelShader.GetBytecode();
	fbPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	fbPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	fbPsoDesc.SampleDesc = sampleDesc;
	fbPsoDesc.SampleMask = 0xffffffff;
	fbPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	fbPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	fbPsoDesc.NumRenderTargets = 1;
	fbPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	// Create Framebuffer PSO
	result = assets->GetDevice()->CreateGraphicsPipelineState(&fbPsoDesc, IID_PPV_ARGS(&fbPipelineStateObject));
	if (FAILED(result))
	{
		return false;
	}

	// Verticies
	Vertex cubeVList[] = {
		// Front
		{ -0.5f,  0.5f, -0.5f, 0.0f, 0.0f },
		{  0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

		// Right
		{  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
		{  0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
		{  0.5f,  0.5f, -0.5f, 0.0f, 0.0f },

		// Left
		{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
		{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f },

		// Back
		{  0.5f,  0.5f,  0.5f, 0.0f, 0.0f },
		{ -0.5f, -0.5f,  0.5f, 1.0f, 1.0f },
		{  0.5f, -0.5f,  0.5f, 0.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f },

		// Top
		{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f,  0.5f, 1.0f, 0.0f },
		{  0.5f,  0.5f, -0.5f, 1.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 0.0f, 0.0f },

		// Bottom
		{  0.5f, -0.5f,  0.5f, 0.0f, 0.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
		{  0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f },
	};
	int cubeVBufferSize = sizeof(cubeVList);

	// Verticies
	Vertex quadVList[] = {
		// Front
		{ -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
		{  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
		{ -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
		{  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
	};
	int quadVBufferSize = sizeof(quadVList);

	// Cube Verticies Default Heap
	CD3DX12_HEAP_PROPERTIES dHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(cubeVBufferSize);
	assets->GetDevice()->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&cubeVertexBuffer));
	cubeVertexBuffer->SetName(L"Cube Vertex Buffer Resource Heap");

	// Cube Verticies Upload Heap
	CD3DX12_HEAP_PROPERTIES uHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ID3D12Resource* cubeVBufferUploadHeap;
	assets->GetDevice()->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cubeVBufferUploadHeap));
	cubeVBufferUploadHeap->SetName(L"Cube Vertex Buffer Upload Resource Heap");

	// Cube Copy Verticies to Default Heap
	D3D12_SUBRESOURCE_DATA cubeVertexData = {};
	cubeVertexData.pData = reinterpret_cast<BYTE*>(cubeVList);
	cubeVertexData.RowPitch = cubeVBufferSize;
	cubeVertexData.SlicePitch = cubeVBufferSize;
	UpdateSubresources(assets->GetCommandList(), cubeVertexBuffer, cubeVBufferUploadHeap, 0, 0, 1, &cubeVertexData);
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(cubeVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	// Quad Verticies Default Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(quadVBufferSize);
	assets->GetDevice()->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&quadVertexBuffer));
	quadVertexBuffer->SetName(L"Quad Vertex Buffer Resource Heap");

	// Quad Verticies Upload Heap
	ID3D12Resource* quadVBufferUploadHeap;
	assets->GetDevice()->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&quadVBufferUploadHeap));
	quadVBufferUploadHeap->SetName(L"Quad Vertex Buffer Upload Resource Heap");

	// Quad Copy Verticies to Default Heap
	D3D12_SUBRESOURCE_DATA quadVertexData = {};
	quadVertexData.pData = reinterpret_cast<BYTE*>(quadVList);
	quadVertexData.RowPitch = quadVBufferSize;
	quadVertexData.SlicePitch = quadVBufferSize;
	UpdateSubresources(assets->GetCommandList(), quadVertexBuffer, quadVBufferUploadHeap, 0, 0, 1, &quadVertexData);
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(quadVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	// Cube Indicies
	UINT32 cubeIList[] = {
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
	int cubeIBufferSize = sizeof(cubeIList);
	numCubeIndices = sizeof(cubeIList) / sizeof(UINT32);

	// Quad Indicies
	UINT32 quadIList[] = {
		// Front
		0, 1, 2,
		0, 3, 1,
	};
	int quadIBufferSize = sizeof(quadIList);

	// Cube Indicies Default Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(cubeIBufferSize);
	assets->GetDevice()->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&cubeIndexBuffer));
	cubeIndexBuffer->SetName(L"Cube Index Buffer Resource Heap");

	// Cube Indicies Upload Heap
	ID3D12Resource* cubeIBufferUploadHeap;
	assets->GetDevice()->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cubeIBufferUploadHeap));
	cubeIBufferUploadHeap->SetName(L"Cube Index Buffer Upload Resource Heap");

	// Cube Copy Indicies to Default Heap
	D3D12_SUBRESOURCE_DATA cubeIndexData = {};
	cubeIndexData.pData = reinterpret_cast<BYTE*>(cubeIList);
	cubeIndexData.RowPitch = cubeIBufferSize;
	cubeIndexData.SlicePitch = cubeIBufferSize;
	UpdateSubresources(assets->GetCommandList(), cubeIndexBuffer, cubeIBufferUploadHeap, 0, 0, 1, &cubeIndexData);
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(cubeIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	// Quad Indicies Default Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(quadIBufferSize);
	assets->GetDevice()->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&quadIndexBuffer));
	quadIndexBuffer->SetName(L"Quad Index Buffer Resource Heap");

	// Quad Indicies Upload Heap
	ID3D12Resource* quadIBufferUploadHeap;
	assets->GetDevice()->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&quadIBufferUploadHeap));
	quadIBufferUploadHeap->SetName(L"Quad Index Buffer Upload Resource Heap");

	// Quad Copy Indicies to Default Heap
	D3D12_SUBRESOURCE_DATA quadIndexData = {};
	quadIndexData.pData = reinterpret_cast<BYTE*>(quadIList);
	quadIndexData.RowPitch = quadIBufferSize;
	quadIndexData.SlicePitch = quadIBufferSize;
	UpdateSubresources(assets->GetCommandList(), quadIndexBuffer, quadIBufferUploadHeap, 0, 0, 1, &quadIndexData);
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(quadIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	// Create Depth Stencil Buffer Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = assets->GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
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
	assets->GetDevice()->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer));
	dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	assets->GetDevice()->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create Constant Buffer Resource Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		result = assets->GetDevice()->CreateCommittedResource(
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

		memcpy(cbvGPUAddress[i], &cbPerObject, sizeof(cbPerObject)); // Cube
	}

	// Load Image From File
	Texture* newTex = textureManager->CreateTexture(L"assets/gato.png");

	// Assert Image Data
	if (newTex->GetSize() <= 0)
	{
		running = false;
		return false;
	}

	// Texture Buffer Default Heap
	result = assets->GetDevice()->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&newTex->GetDesc(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&textureBuffer));
	if (FAILED(result))
	{
		running = false;
		return false;
	}
	textureBuffer->SetName(L"Texture Buffer Resource Heap");

	UINT64 textureUploadBufferSize;
	assets->GetDevice()->GetCopyableFootprints(&newTex->GetDesc(), 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// Texture Buffer Upload Heap
	resoDesc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
	result = assets->GetDevice()->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureBufferUploadHeap));
	if (FAILED(result))
	{
		running = false;
		return false;
	}
	textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// Copy Texture Data To Default Heap
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &newTex->GetData()[0];
	textureData.RowPitch = newTex->GetBytesPerRow();
	textureData.SlicePitch = newTex->GetBytesPerRow() * newTex->GetDesc().Height;
	UpdateSubresources(assets->GetCommandList(), textureBuffer, textureBufferUploadHeap, 0, 0, 1, &textureData);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	// Create SRV Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = assets->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap));
	if (FAILED(result))
	{
		running = false;
	}

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = newTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	assets->GetDevice()->CreateShaderResourceView(textureBuffer, &srvDesc, mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create Font Descriptor Heap
	result = assets->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&fontDescriptorHeap));
	if (FAILED(result))
	{
		running = false;
	}
	Renderer::fontDescriptorHeapAlloc.Create(assets->GetDevice(), fontDescriptorHeap);

	// Execute Command List
	assets->GetCommandList()->Close();
	ID3D12CommandList* ppCommandLists[] = { assets->GetCommandList() };
	assets->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Increment Fence Value
	assets->IncrementFenceValue(assets->GetFrameIndex());
	result = assets->GetCommandQueue()->Signal(assets->GetFence(assets->GetFrameIndex()), assets->GetFenceValue(assets->GetFrameIndex()));
	if (FAILED(result))
	{
		return false;
	}

	// Free Obsolete Texture Data
	textureManager->Cleanup();

	// Create VBV
	cubeVertexBufferView.BufferLocation = cubeVertexBuffer->GetGPUVirtualAddress();
	cubeVertexBufferView.StrideInBytes = sizeof(Vertex);
	cubeVertexBufferView.SizeInBytes = cubeVBufferSize;

	// Create Index Buffer View
	cubeIndexBufferView.BufferLocation = cubeIndexBuffer->GetGPUVirtualAddress();
	cubeIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	cubeIndexBufferView.SizeInBytes = cubeIBufferSize;

	// Create VBV
	quadVertexBufferView.BufferLocation = quadVertexBuffer->GetGPUVirtualAddress();
	quadVertexBufferView.StrideInBytes = sizeof(Vertex);
	quadVertexBufferView.SizeInBytes = quadVBufferSize;

	// Create Index Buffer View
	quadIndexBufferView.BufferLocation = quadIndexBuffer->GetGPUVirtualAddress();
	quadIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	quadIndexBufferView.SizeInBytes = quadIBufferSize;

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
	XMStoreFloat4x4(&cube1DefaultRotMat, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&cube1RotMat, DirectX::XMMatrixIdentity());
	XMStoreFloat4x4(&cube1WorldMat, tmpMat);

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = assets->GetDevice();
	init_info.CommandQueue = assets->GetCommandQueue();
	init_info.NumFramesInFlight = FRAME_BUFFER_COUNT;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	init_info.SrvDescriptorHeap = mainDescriptorHeap;
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return fontDescriptorHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return fontDescriptorHeapAlloc.Free(cpu_handle, gpu_handle); };
	if (!ImGui_ImplDX12_Init(&init_info)) {
		return false;
	}

	dsaModifiers = {1.0f, 1.0f, 1.0f};

	return true;
}

void Renderer::UnInit()
{
	// Wait for GPU to finish
	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		assets->SetFrameIndex(i);
		WaitForPreviousFrame();
	}

	assets->UnInit();
	delete assets;
	assets = nullptr;

	delete textureManager;
	textureManager = nullptr;

	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(cubeVertexBuffer);
	SAFE_RELEASE(cubeIndexBuffer);

	SAFE_RELEASE(depthStencilBuffer);
	SAFE_RELEASE(dsDescriptorHeap);

	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		SAFE_RELEASE(constantBufferUploadHeaps[i]);
	};

	ImGui_ImplDX12_Shutdown();
}

void Renderer::Update(float dt)
{
	// add rotation to cube1's rotation matrix and store it
	DirectX::XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat);
	if (rotateX) {
		DirectX::XMMATRIX rotMatX = XMMatrixRotationX(rotateSpeed * 0.002f * dt);
		rotMat *= rotMatX;
	}
	if (rotateY) {
		DirectX::XMMATRIX rotMatY = XMMatrixRotationY(rotateSpeed * 0.002f * dt);
		rotMat *= rotMatY;
	}
	if (rotateZ) {
		DirectX::XMMATRIX rotMatZ = XMMatrixRotationZ(rotateSpeed * 0.002f * dt);
		rotMat *= rotMatZ;
	}
	if (resetCube) {
		rotateX = false;
		rotateY = false;
		rotateZ = false;
		rotMat = XMLoadFloat4x4(&cube1DefaultRotMat);
	}
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
	DirectX::XMMATRIX wMat = XMLoadFloat4x4(&cube1WorldMat); // create world matrix
	DirectX::XMMATRIX vpMat = viewMat * projMat; // create view projection matrix
	DirectX::XMMATRIX transposed = XMMatrixTranspose(wMat); // must transpose w matrix for the gpu
	XMStoreFloat4x4(&cbPerObject.wMat, transposed); // store transposed w matrix in constant buffer
	transposed = XMMatrixTranspose(vpMat); // must transpose vp matrix for the gpu
	XMStoreFloat4x4(&cbPerObject.vpMat, transposed); // store transposed vp matrix in constant buffer
	DirectX::XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
	XMStoreFloat4(&cbPerObject.camPos, cPos);
	DirectX::XMVECTOR dsa = XMLoadFloat3(&dsaModifiers);
	XMStoreFloat3(&cbPerObject.dsaMod, dsa);

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(cbvGPUAddress[assets->GetFrameIndex()], &cbPerObject, sizeof(cbPerObject));
}

void Renderer::UpdatePipeline()
{
	HRESULT result;

	// Wait for GPU to finish
	WaitForPreviousFrame();

	// Reset allocator when GPU is done
	result = assets->GetCommandAllocator(assets->GetFrameIndex())->Reset();
	if (FAILED(result)) {
		running = false;
	}

	// Reset command lists and set starting PSO
	result = assets->GetCommandList()->Reset(assets->GetCommandAllocator(assets->GetFrameIndex()), pipelineStateObject);
	if (FAILED(result)) {
		running = false;
	}

	// Reset render target to write
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(assets->GetRenderTarget(assets->GetFrameIndex()), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	// Get handle to render target and depth buffer for merger stage
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(assets->GetRtvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), assets->GetFrameIndex(), assets->GetRtvDescriptorSize());
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	assets->GetCommandList()->RSSetViewports(1, &viewport);
	assets->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	assets->GetCommandList()->SetGraphicsRootSignature(rootSignature);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap };
	assets->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	const float clearColor[] = {0.2f, 0.1f, 0.3f, 1.0f};
	assets->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	assets->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	assets->GetCommandList()->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	assets->GetCommandList()->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	
	// Render Scene
	assets->GetCommandList()->SetPipelineState(pipelineStateObject);
	assets->GetCommandList()->IASetVertexBuffers(0, 1, &cubeVertexBufferView);
	assets->GetCommandList()->IASetIndexBuffer(&cubeIndexBufferView);
	assets->GetCommandList()->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[assets->GetFrameIndex()]->GetGPUVirtualAddress());
	assets->GetCommandList()->DrawIndexedInstanced(numCubeIndices, 1, 0, 0, 0);

	// Render Quad
	//assets->GetCommandList()->SetPipelineState(fbPipelineStateObject);
	//assets->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//assets->GetCommandList()->IASetVertexBuffers(0, 1, &quadVertexBufferView);
	//assets->GetCommandList()->IASetIndexBuffer(&quadIndexBufferView);
	//assets->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

	RenderImGui();

	// Set render target to present state
	resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(assets->GetRenderTarget(assets->GetFrameIndex()), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	assets->GetCommandList()->ResourceBarrier(1, &resoBarr);

	result = assets->GetCommandList()->Close();
	if (FAILED(result)) {
		running = false;
	}
}

void Renderer::Render()
{
	HRESULT result;

	UpdatePipeline();

	// Create array of command lists
	ID3D12CommandList* ppCommandLists[] = { assets->GetCommandList() };

	// Execute command lists
	assets->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Last command in queue
	result = assets->GetCommandQueue()->Signal(assets->GetFence(assets->GetFrameIndex()), assets->GetFenceValue(assets->GetFrameIndex()));
	if (FAILED(result)) {
		running = false;
	}

	// Present the backbuffer
	result = assets->GetSwapChain()->Present(0, 0);
	if (FAILED(result)) {
		running = false;
	}
}

void Renderer::WaitForPreviousFrame()
{
	HRESULT result;

	// Swap to correct buffer
	assets->SetFrameIndex(assets->GetSwapChain()->GetCurrentBackBufferIndex());

	// Check if GPU has finished executing
	if (assets->GetFence(assets->GetFrameIndex())->GetCompletedValue() < assets->GetFenceValue(assets->GetFrameIndex()))
	{
		// Fence create an event
		result = assets->GetFence(assets->GetFrameIndex())->SetEventOnCompletion(assets->GetFenceValue(assets->GetFrameIndex()), assets->GetFenceEvent());
		if (FAILED(result)) {
			running = false;
		}

		// Wait for event to finish
		WaitForSingleObject(assets->GetFenceEvent(), INFINITE);
	}

	// Increment for next frame
	assets->IncrementFenceValue(assets->GetFrameIndex());
}

void Renderer::CloseFenceEventHandle() { CloseHandle(assets->GetFenceEvent()); }

void Renderer::RenderImGui()
{
	// Render ImGui
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Menu");
	if (ImGui::CollapsingHeader("Lighting Settings")) {
		ImGui::SliderFloat("Diffuse", &dsaModifiers.x, 0.0f, 1.0f);
		ImGui::SliderFloat("Specular", &dsaModifiers.y, 0.0f, 1.0f);
		ImGui::SliderFloat("Ambient", &dsaModifiers.z, 0.0f, 1.0f);
	}
	if (ImGui::CollapsingHeader("Cube Settings")) {
		resetCube = ImGui::Button("Reset Cube");
		ImGui::Checkbox("Rotate X", &rotateX);
		ImGui::Checkbox("Rotate Y", &rotateY);
		ImGui::Checkbox("Rotate Z", &rotateZ);
		ImGui::SliderFloat("Rotate Speed", &rotateSpeed, 0.0f, 1.0f);
	}
	ImGui::End();

	ImGui::Render();
	assets->GetCommandList()->SetDescriptorHeaps(1, &fontDescriptorHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), assets->GetCommandList());
}

bool Shader::Init(LPCWSTR filename, LPCSTR entryFunc, LPCSTR target)
{
	HRESULT result;

	result = D3DCompileFromFile(filename, nullptr, nullptr,
		entryFunc, target, 0,
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

void DescriptorHeapAllocator::Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
{
	IM_ASSERT(Heap == nullptr && FreeIndices.empty());
	Heap = heap;
	D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
	HeapType = desc.Type;
	HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
	HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
	HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
	FreeIndices.reserve((int)desc.NumDescriptors);
	for (int n = desc.NumDescriptors; n > 0; n--)
		FreeIndices.push_back(n);
}

void DescriptorHeapAllocator::Destroy()
{
	Heap = nullptr;
	FreeIndices.clear();
}

void DescriptorHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
	IM_ASSERT(FreeIndices.Size > 0);
	int idx = FreeIndices.back();
	FreeIndices.pop_back();
	out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
	out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
}

void DescriptorHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
{
	int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
	int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
	IM_ASSERT(cpu_idx == gpu_idx);
	FreeIndices.push_back(cpu_idx);
}
