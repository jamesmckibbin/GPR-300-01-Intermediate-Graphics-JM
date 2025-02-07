#pragma once

#include "gconst.h"

class Texture {

public:

	bool LoadTextureDataFromFile(LPCWSTR filename);
	void CleanObsoleteTextureData();

	D3D12_RESOURCE_DESC GetDesc() { return textureDesc; }
	int GetBytesPerRow() { return bytesPerRow; }
	BYTE* GetData() { return textureData; }
	int GetSize() { return textureSize; }

private:

	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

	LPCWSTR name;
	D3D12_RESOURCE_DESC textureDesc;
	int bytesPerRow;
	BYTE* textureData;
	int textureSize;

	friend class TextureManager;

};

class TextureManager {

public:

	Texture* CreateTexture(LPCWSTR filename);
	Texture* GetTextureByFileName(LPCWSTR filename);
	void Cleanup();

private:

	std::vector<Texture*> textures;

};