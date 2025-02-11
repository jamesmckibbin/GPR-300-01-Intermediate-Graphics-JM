#pragma once

#include "gconst.h"
#include "shader.h"

class PipelineStateObject
{
public:

	~PipelineStateObject();
	bool Init(ID3D12Device* device, ID3D12RootSignature* rootSignature, const D3D12_INPUT_ELEMENT_DESC* inputLayout, Shader& vs, Shader& ps);

	ID3D12PipelineState* GetState() { return pso; }

private:

	ID3D12PipelineState* pso;

};