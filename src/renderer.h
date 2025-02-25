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
#include "shader.h"
#include "pipelinestateobject.h"

struct ConstantBufferPerObject {
	DirectX::XMFLOAT4X4 wMat;
	DirectX::XMFLOAT4X4 vpMat;
	DirectX::XMFLOAT4X4 lMat;
	DirectX::XMFLOAT4 lDir;
	DirectX::XMFLOAT4 camPos;

	DirectX::XMFLOAT3 dsaMod;
	UINT32 ppOption;
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

	void CreateUploadVIData();
	bool CreatePipelineStateObjects();
	void RenderImGui();
	void DrawScene(bool drawCube, bool drawPlane);

	RenderAssets* assets;
	TextureManager* textureManager;
	ResourceManager* resourceManager;

	ID3D12Resource* renderTexture;
	ID3D12DescriptorHeap* rtDescriptorHeap;

	// Root Signature & Pipeline State Object
	ID3D12RootSignature* baseRootSig;
	PipelineStateObject* scenePSO;
	PipelineStateObject* postPSO;
	PipelineStateObject* shadowPSO;

	// Vertex & Index Buffers
	ID3D12Resource* cubeVertexBuffer;
	ID3D12Resource* cubeIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW cubeVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW cubeIndexBufferView;
	ID3D12Resource* renderTriVertexBuffer;
	ID3D12Resource* renderTriIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW renderTriVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW renderTriIndexBufferView;
	ID3D12Resource* planeVertexBuffer;
	ID3D12Resource* planeIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW planeVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW planeIndexBufferView;

	// Depth Buffer
	ID3D12Resource* depthStencilBuffer;
	ID3D12Resource* shadowMapBuffer;
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
	DirectX::XMFLOAT4X4 cubeWorldMat;
	DirectX::XMFLOAT4X4 cubeDefaultRotMat;
	DirectX::XMFLOAT4X4 cubeRotMat;
	DirectX::XMFLOAT4 cubePosition;
	int numCubeIndices;

	DirectX::XMFLOAT4X4 planeWorldMat;
	DirectX::XMFLOAT4 planePosition;

	// Textures
	ID3D12Resource* textureBuffer;
	ID3D12DescriptorHeap* srvDescriptorHeap;

	// ImGui Reqs
	ID3D12DescriptorHeap* fontDescriptorHeap;
	static DescriptorHeapAllocator fontDescriptorHeapAlloc;
	DirectX::XMFLOAT3 dsaModifiers;
	UINT32 ppOption;
	bool rotateX = false, rotateY = false, rotateZ = false;
	float rotateSpeed = 1.0f;
	bool resetCube = false;
	DirectX::XMFLOAT4 lightPosition = {2.0f, 2.0f, -2.0f, 0.0f};
	float nearPlane = 1.0f, farPlane = 7.5f;
	
	// Misc Draw Data
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	D3D12_VIEWPORT smViewport;
	D3D12_RECT smScissorRect;

};