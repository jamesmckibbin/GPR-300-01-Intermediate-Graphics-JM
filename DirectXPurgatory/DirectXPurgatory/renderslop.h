#pragma once

/*
Thank you so much to these websites for helping me set up this project.
https://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
https://github.com/galek/SDL-Directx12
*/

#include "gconst.h"

const int frameBufferCount = 2;

struct Vertex {
	Vertex(float x, float y, float z, float r, float g, float b, float a, float u, float v) : pos(x, y, z), color(r, g, b, a), tex(u, v) {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT2 tex;
};

struct ConstantBuffer {
	DirectX::XMFLOAT4 colorMultiplier;
};

class RenderSlop {
public:
	bool Init(const HWND& window, bool screenState, float width, float height);
	void UnInit();
	void Update();
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

	// Verticies & Indicies
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// Depth Buffer
	ID3D12Resource* depthStencilBuffer;
	ID3D12DescriptorHeap* dsDescriptorHeap;

	// Constant Buffer
	ID3D12DescriptorHeap* mainDescriptorHeap[frameBufferCount];
	ID3D12Resource* constantBufferUploadHeap[frameBufferCount];
	ConstantBuffer cbColorMultiplierData;
	UINT8* cbColorMultiplierGPUAddress[frameBufferCount];
	
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