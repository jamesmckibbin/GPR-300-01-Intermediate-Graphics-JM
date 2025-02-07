#pragma once

#include "gconst.h"

class ResourceManager
{

public:
	~ResourceManager();
	ID3D12Resource* CreateDefaultHeap(ID3D12Device* device, ID3D12Resource* resource, LPCWSTR resourceName, int bufferSize);
	ID3D12Resource* CreateUploadHeap(ID3D12Device* device, LPCWSTR resourceName, int bufferSize);

private:

	std::vector<ID3D12Resource*> bufferUploadHeaps;

};