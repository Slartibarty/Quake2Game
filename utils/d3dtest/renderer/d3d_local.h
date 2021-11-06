
// Shared definitions for the Direct3D 11 renderer
// dx headers must be pre-included

#pragma once

#include "../../../core/core.h"

#include <vector>

#include "../../../framework/filesystem.h"
#include "../../../framework/cmdsystem.h"
#include "../../../framework/cvarsystem.h"

#include "../../../thirdparty/DirectXMath/Inc/DirectXMath.h"

#include "../../../core/sys_includes.h"
#include <dxgi1_6.h>
#include <d3d11_4.h>
#ifdef Q_DEBUG
#include <dxgidebug.h>
#endif

#include "d3d_public.h"

namespace Renderer
{

struct alignas( 16 ) shaderConstants_t
{
	DirectX::XMFLOAT4X4A viewMatrix;
	DirectX::XMFLOAT4X4A projMatrix;
};

struct d3dDefs_t
{
	// DXGI
	IDXGIFactory7 *				factory;
	IDXGISwapChain1 *			swapChain;

	// D3D11
	ID3D11Device *				device;
	ID3D11DeviceContext *		immediateContext;
	ID3D11RenderTargetView *	renderTargetView;
	ID3D11DepthStencilView *	depthStencilView;

	// Objects

	// Shader gubbins
	ID3D11Buffer *				constantBuffer;
	ID3D11VertexShader *		vertexShader;
	ID3D11PixelShader *			pixelShader;
	ID3D11InputLayout *			inputLayout;
	// States
	ID3D11RasterizerState *		rastState;
	ID3D11DepthStencilState *	depthStencilState;
};

extern d3dDefs_t d3d;

struct mesh_t
{
	ID3D11Buffer *vertexBuffer;
	ID3D11Buffer *indexBuffer;
	uint32 numVertices;
	uint32 numIndices;
};

struct meshManager_t
{
	std::vector<mesh_t> meshes;
};

extern meshManager_t meshMan;

} // namespace Renderer
