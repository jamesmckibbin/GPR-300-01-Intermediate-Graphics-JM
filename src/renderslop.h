#pragma once

/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "gconst.h"

const int frameBufferCount = 3;

struct Vertex {
	Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), normals(x, y, z), texCoord(u, v) {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normals;
	DirectX::XMFLOAT2 texCoord;
};

struct ConstantBufferPerObject {
	DirectX::XMFLOAT4X4 wMat;
	DirectX::XMFLOAT4X4 vpMat;
	DirectX::XMFLOAT4 camPos;
};

class RenderSlop {
public:
	bool Init(const HWND& window, bool screenState, float width, float height);
	void UnInit();
	void Update(float dt);
	void UpdatePipeline();
	void Render();
	void WaitForPreviousFrame();
	void CloseFenceEventHandle();
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
	ID3D12CommandAllocator* commandAllocator[frameBufferCount];
	ID3D12GraphicsCommandList* commandList;
	ID3D12Fence* fence[frameBufferCount];
	HANDLE fenceEvent;
	UINT64 fenceValue[frameBufferCount];

	// Render Target View
	ID3D12DescriptorHeap* rtvDescriptorHeap;
	ID3D12Resource* renderTargets[frameBufferCount];
	int frameIndex;
	int rtvDescriptorSize;

	// Root Signature & Pipeline State Object
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipelineStateObject;

	// Vertex & Index Buffers
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// Depth Buffer
	ID3D12Resource* depthStencilBuffer;
	ID3D12DescriptorHeap* dsDescriptorHeap;

	// Constant Buffer
	int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;
	ConstantBufferPerObject cbPerObject;
	ID3D12Resource* constantBufferUploadHeaps[frameBufferCount];
	UINT8* cbvGPUAddress[frameBufferCount];

	// Camera
	DirectX::XMFLOAT4X4 cameraProjMat;
	DirectX::XMFLOAT4X4 cameraViewMat;
	DirectX::XMFLOAT4 cameraPosition;
	DirectX::XMFLOAT4 cameraTarget;
	DirectX::XMFLOAT4 cameraUp;

	// Cubes
	DirectX::XMFLOAT4X4 cube1WorldMat;
	DirectX::XMFLOAT4X4 cube1RotMat;
	DirectX::XMFLOAT4 cube1Position;
	DirectX::XMFLOAT4X4 cube2WorldMat;
	DirectX::XMFLOAT4X4 cube2RotMat;
	DirectX::XMFLOAT4 cube2PositionOffset;
	int numCubeIndices;

	// Textures
	ID3D12Resource* textureBuffer;
	int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);
	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
	ID3D12DescriptorHeap* mainDescriptorHeap;
	ID3D12Resource* textureBufferUploadHeap;
	
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