#include "shader.h"

bool Shader::Init(LPCWSTR filename, LPCSTR entryFunc, LPCSTR target)
{
	HRESULT result;

	result = D3DCompileFromFile(filename, nullptr, nullptr,
		entryFunc, target, 0,
		0, &shaderBlob, &errorBlob);
	if (FAILED(result))
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		return false;
	}

	shaderBytecode = {};
	shaderBytecode.BytecodeLength = shaderBlob->GetBufferSize();
	shaderBytecode.pShaderBytecode = shaderBlob->GetBufferPointer();

	return true;
}

ID3DBlob* Shader::GetBlob() { return shaderBlob; }

ID3DBlob* Shader::GetErrorBlob() { return errorBlob; }

D3D12_SHADER_BYTECODE Shader::GetBytecode() { return shaderBytecode; }