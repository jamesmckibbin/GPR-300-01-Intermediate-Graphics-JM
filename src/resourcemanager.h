#pragma once

#include "gconst.h"
#include "texturemanager.h"

class ResourceManager {

public:
	~ResourceManager();
	ID3D12Resource* CreateVIDefaultHeap(ID3D12Device* device, ID3D12Resource* resource, LPCWSTR resourceName, int bufferSize);
	ID3D12Resource* CreateVIUploadHeap(ID3D12Device* device, LPCWSTR resourceName, int bufferSize);
	void UploadVertexResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, ID3D12Resource* uploadResource, Vertex* list);
	void UploadIndexResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, ID3D12Resource* uploadResource, UINT32* list);

	ID3D12Resource* CreateTexDefaultHeap(ID3D12Device* device, ID3D12Resource* resource, Texture* tex, LPCWSTR resourceName, int bufferSize);
	ID3D12Resource* CreateTexUploadHeap(ID3D12Device* device, Texture* tex, LPCWSTR resourceName, int bufferSize);
	void UploadTextureResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, ID3D12Resource* uploadResource, Texture* tex);

private:

	std::vector<ID3D12Resource*> bufferUploadHeaps;

};