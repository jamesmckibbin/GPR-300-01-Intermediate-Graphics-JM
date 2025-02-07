#pragma once

/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "gconst.h"
#include "renderassets.h"
#include "texturemanager.h"
#include "resourcemanager.h"

struct ConstantBufferPerObject {
	DirectX::XMFLOAT4X4 wMat;
	DirectX::XMFLOAT4X4 vpMat;
	DirectX::XMFLOAT4 camPos;
	DirectX::XMFLOAT3 dsaMod;
};

// From DirectX12 ImGui Examples
struct DescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
	void Destroy();
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);
};

class Renderer {
public:
	bool Init(const HWND& window, bool screenState, float width, float height);
	void UnInit();
	void Update(float dt);
	void UpdatePipeline();
	void Render();
	void WaitForPreviousFrame();
	void CloseFenceEventHandle();
private:

	void RenderImGui();

	RenderAssets* assets;
	TextureManager* textureManager;
	ResourceManager* resourceManager;

	ID3D12Resource* frameBuffer;

	// Root Signature & Pipeline State Object
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipelineStateObject;
	ID3D12PipelineState* fbPipelineStateObject;

	// Vertex & Index Buffers
	ID3D12Resource* cubeVertexBuffer;
	ID3D12Resource* cubeIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW cubeVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW cubeIndexBufferView;

	ID3D12Resource* quadVertexBuffer;
	ID3D12Resource* quadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW quadIndexBufferView;

	// Depth Buffer
	ID3D12Resource* depthStencilBuffer;
	ID3D12DescriptorHeap* dsDescriptorHeap;

	// Constant Buffer
	int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;
	ConstantBufferPerObject cbPerObject;
	ID3D12Resource* constantBufferUploadHeaps[FRAME_BUFFER_COUNT];
	UINT8* cbvGPUAddress[FRAME_BUFFER_COUNT];

	// Camera
	DirectX::XMFLOAT4X4 cameraProjMat;
	DirectX::XMFLOAT4X4 cameraViewMat;
	DirectX::XMFLOAT4 cameraPosition;
	DirectX::XMFLOAT4 cameraTarget;
	DirectX::XMFLOAT4 cameraUp;

	// Cube
	DirectX::XMFLOAT4X4 cube1WorldMat;
	DirectX::XMFLOAT4X4 cube1DefaultRotMat;
	DirectX::XMFLOAT4X4 cube1RotMat;
	DirectX::XMFLOAT4 cube1Position;
	int numCubeIndices;

	// Textures
	ID3D12Resource* textureBuffer;
	ID3D12DescriptorHeap* mainDescriptorHeap;
	ID3D12Resource* textureBufferUploadHeap;

	// ImGui Reqs
	ID3D12DescriptorHeap* fontDescriptorHeap;
	static DescriptorHeapAllocator fontDescriptorHeapAlloc;
	DirectX::XMFLOAT3 dsaModifiers;
	bool rotateX, rotateY, rotateZ;
	float rotateSpeed = 1.0f;
	bool resetCube;
	
	// Misc Draw Data
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

};

class Shader {
public:
	bool Init(LPCWSTR filename, LPCSTR entryFunc, LPCSTR target);
	ID3DBlob* GetBlob();
	ID3DBlob* GetErrorBlob();
	D3D12_SHADER_BYTECODE GetBytecode();

private:
	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	D3D12_SHADER_BYTECODE shaderBytecode;
};