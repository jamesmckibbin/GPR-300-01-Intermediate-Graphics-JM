#include "resourcemanager.h"

ResourceManager::~ResourceManager()
{
	for (ID3D12Resource* heaps : bufferUploadHeaps)
	{
		SAFE_RELEASE(heaps);
	}
}

ID3D12Resource* ResourceManager::CreateVIDefaultHeap(ID3D12Device* device, ID3D12Resource* resource, LPCWSTR resourceName, int bufferSize)
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

ID3D12Resource* ResourceManager::CreateVIUploadHeap(ID3D12Device* device, LPCWSTR resourceName, int bufferSize)
{
	ID3D12Resource* viBUH = {};
	bufferUploadHeaps.push_back(viBUH);
	CD3DX12_HEAP_PROPERTIES uHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	device->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&viBUH));
	viBUH->SetName(resourceName);

	return viBUH;
}

void ResourceManager::UploadVertexResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, ID3D12Resource* uploadResource, Vertex* list)
{
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	D3D12_SUBRESOURCE_DATA resourceData = {};
	resourceData.pData = reinterpret_cast<BYTE*>(list);
	resourceData.RowPitch = sizeof(list);
	resourceData.SlicePitch = sizeof(list);

	UpdateSubresources(commandList, resource, uploadResource, 0, 0, 1, &resourceData);
	commandList->ResourceBarrier(1, &resoBarr);
}

void ResourceManager::UploadIndexResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, ID3D12Resource* uploadResource, UINT32* list)
{
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	D3D12_SUBRESOURCE_DATA resourceData = {};
	resourceData.pData = reinterpret_cast<BYTE*>(list);
	resourceData.RowPitch = sizeof(list);
	resourceData.SlicePitch = sizeof(list);

	UpdateSubresources(commandList, resource, uploadResource, 0, 0, 1, &resourceData);
	commandList->ResourceBarrier(1, &resoBarr);
}

ID3D12Resource* ResourceManager::CreateTexDefaultHeap(ID3D12Device* device, ID3D12Resource* resource, Texture* tex, LPCWSTR resourceName, int bufferSize)
{
	CD3DX12_HEAP_PROPERTIES dHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	device->CreateCommittedResource(
		&dHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&tex->GetDesc(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource));
	resource->SetName(resourceName);

	return resource;
}

ID3D12Resource* ResourceManager::CreateTexUploadHeap(ID3D12Device* device, Texture* tex, LPCWSTR resourceName, int bufferSize)
{
	UINT64 textureUploadBufferSize;
	device->GetCopyableFootprints(&tex->GetDesc(), 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	ID3D12Resource* texBUH = {};
	bufferUploadHeaps.push_back(texBUH);

	CD3DX12_HEAP_PROPERTIES uHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resoDesc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);

	device->CreateCommittedResource(
		&uHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resoDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBUH));
	texBUH->SetName(resourceName);

	return texBUH;
}

void ResourceManager::UploadTextureResources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, ID3D12Resource* uploadResource, Texture* tex)
{
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &tex->GetData()[0];
	textureData.RowPitch = tex->GetBytesPerRow();
	textureData.SlicePitch = tex->GetBytesPerRow() * tex->GetDesc().Height;

	UpdateSubresources(commandList, resource, uploadResource, 0, 0, 1, &textureData);
	CD3DX12_RESOURCE_BARRIER resoBarr = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->ResourceBarrier(1, &resoBarr);
}
