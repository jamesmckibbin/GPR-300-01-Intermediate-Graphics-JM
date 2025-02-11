#pragma once

#include "gconst.h"

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