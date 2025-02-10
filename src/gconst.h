#pragma once

// WIC and DirectX 12
#include <wincodec.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

// SDL3
#include <SDL3/SDL.h>

// ImGui
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl3.h>

#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

#define FRAME_BUFFER_COUNT 2

struct Vertex {
	Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), normals(x, y, z), texCoord(u, v) {}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normals;
	DirectX::XMFLOAT2 texCoord;
};