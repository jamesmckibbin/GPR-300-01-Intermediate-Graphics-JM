#include "resourcemanager.h"

ResourceManager::~ResourceManager()
{
	for (ID3D12Resource* heaps : bufferUploadHeaps)
	{
		SAFE_RELEASE(heaps);
	}
}

ID3D12Resource* ResourceManager::CreateDefaultHeap(ID3D12Device* device, ID3D12Resource* resource, LPCWSTR resourceName, int bufferSize)
{
	CD3DX12_HEAP_PROPERTIES dHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	device->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource));
	resource->SetName(resourceName);

	return resource;
}

ID3D12Resource* ResourceManager::CreateUploadHeap(ID3D12Device* device, LPCWSTR resourceName, int bufferSize)
{
	ID3D12Resource* newBUH = {};
	bufferUploadHeaps.push_back(newBUH);
	CD3DX12_HEAP_PROPERTIES uHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	device->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&newBUH));
	newBUH->SetName(resourceName);

	return newBUH;
}
