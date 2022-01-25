
#include "d3d_local.h"

#include <d3dcompiler.h>

namespace Renderer
{

#define MY_FLIP_MODEL DXGI_SWAP_EFFECT_DISCARD // DXGI_SWAP_EFFECT_FLIP_DISCARD
#define SHADER_FILE_NAME "shaders/shader.hlsl"

d3dDefs_t d3d;

void Init()
{
	HRESULT hr;

	//=========================================================================
	// DXGI Factory

	UINT factoryFlags = 0;
#ifdef Q_DEBUG
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hr = CreateDXGIFactory2( factoryFlags, __uuidof( IDXGIFactory7 ), (void **)&d3d.factory );
	assert( SUCCEEDED( hr ) );

	//=========================================================================
	// Device & Device Context

	const D3D_FEATURE_LEVEL requestedLevel = D3D_FEATURE_LEVEL_11_1;
	D3D_FEATURE_LEVEL actualLevel;

	UINT deviceFlags = 0;
#ifdef Q_DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE, // Just give me any hardware adapter
		nullptr,
		deviceFlags,
		&requestedLevel,
		1,
		D3D11_SDK_VERSION,
		&d3d.device,
		&actualLevel,
		&d3d.immediateContext
	);
	assert( SUCCEEDED( hr ) );
	assert( actualLevel == requestedLevel );

	//=========================================================================
	// Swap Chain

	const UINT windowWidth = (UINT)wnd_width.GetInt();
	const UINT windowHeight = (UINT)wnd_height.GetInt();
	const HWND windowHandle = (HWND)Sys_GetMainWindow();

	const DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor{
		.Width = windowWidth,
		.Height = windowHeight,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.Stereo = FALSE,
		.SampleDesc
		{
			.Count = 1,
			.Quality = 0
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 1,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = MY_FLIP_MODEL,
		.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags = 0
	};

	hr = d3d.factory->CreateSwapChainForHwnd(
		d3d.device,
		windowHandle,
		&swapChainDescriptor,
		nullptr,
		nullptr,
		&d3d.swapChain
	);
	assert( SUCCEEDED( hr ) );

	//=========================================================================
	// Create / configure backbuffer and depthbuffer

	D3D11_TEXTURE2D_DESC depthBufferDesc; // For later

	// Back Buffer

	// Get the backbuffer texture
	ID3D11Texture2D *backBuffer;
	d3d.swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void **)&backBuffer );
	backBuffer->GetDesc( &depthBufferDesc ); // Save our descriptor

	// Create a view of the backbuffer texture
	d3d.device->CreateRenderTargetView( backBuffer, nullptr, &d3d.renderTargetView );
	backBuffer->Release(); // Thanks for the info

	// Set our render target (is this necessary?)
	d3d.immediateContext->OMSetRenderTargets( 1, &d3d.renderTargetView, nullptr );

	// Depth Buffer

	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	// Create our depthbuffer
	ID3D11Texture2D *depthBuffer;
	d3d.device->CreateTexture2D( &depthBufferDesc, nullptr, &depthBuffer );

	//D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	// Create a view into our depthbuffer
	d3d.device->CreateDepthStencilView( depthBuffer, nullptr, &d3d.depthStencilView );
	depthBuffer->Release(); // Thanks for the info, you're handled by the system now, right?

	//=========================================================================
	// Set-up the viewport, this will need to move when we add window resizing

	const D3D11_VIEWPORT viewport{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = static_cast<float>( windowWidth ),
		.Height = static_cast<float>( windowHeight ),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f
	};
	d3d.immediateContext->RSSetViewports( 1, &viewport );

	//=========================================================================
	// Shaders

	// Create the buffer for our shader constants
	//

	const D3D11_BUFFER_DESC constantBufferDesc{
		.ByteWidth = sizeof( shaderConstants_t ),
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};
	d3d.device->CreateBuffer( &constantBufferDesc, nullptr, &d3d.constantBuffer );

	// Compile shaders
	//

	UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef Q_DEBUG
	compileFlags |= D3DCOMPILE_DEBUG;
#endif

	void *shaderData;
	fsSize_t shaderSize;
	ID3DBlob *vsBlob;
	ID3DBlob *psBlob;
	ID3DBlob *errorBlob;

	shaderSize = FileSystem::LoadFile( SHADER_FILE_NAME, &shaderData );

	hr = D3DCompile( shaderData, shaderSize, SHADER_FILE_NAME, nullptr, nullptr,
		"vs_main", "vs_5_0", compileFlags, 0, &vsBlob, &errorBlob );

	FileSystem::FreeFile( shaderData );

	if ( FAILED( hr ) )
	{
		if ( errorBlob )
		{
			Com_Print( (char *)errorBlob->GetBufferPointer() );
			errorBlob->Release();
		}

		// Bail
		Com_FatalError( "Vertex shader failed to compile\n" );
	}

	shaderSize = FileSystem::LoadFile( SHADER_FILE_NAME, &shaderData );

	hr = D3DCompile( shaderData, shaderSize, SHADER_FILE_NAME, nullptr, nullptr,
		"ps_main", "ps_5_0", compileFlags, 0, &psBlob, &errorBlob );

	FileSystem::FreeFile( shaderData );

	if ( FAILED( hr ) )
	{
		if ( errorBlob )
		{
			Com_Print( (char *)errorBlob->GetBufferPointer() );
			errorBlob->Release();
		}

		// Bail
		Com_FatalError( "Pixel shader failed to compile\n" );
	}

	// Create shader objects
	//

	hr = d3d.device->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		nullptr, &d3d.vertexShader );
	assert( SUCCEEDED( hr ) );

	hr = d3d.device->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
		nullptr, &d3d.pixelShader );
	assert( SUCCEEDED( hr ) );

	// Create shader input element descriptions
	//

	const D3D11_INPUT_ELEMENT_DESC inputElementDescs[]{
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = d3d.device->CreateInputLayout( inputElementDescs, countof( inputElementDescs ),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &d3d.inputLayout );
	assert( SUCCEEDED( hr ) );

	//=========================================================================
	// Rasteriser State

	const D3D11_RASTERIZER_DESC rastDesc{
		.FillMode = D3D11_FILL_WIREFRAME,
		.CullMode = D3D11_CULL_NONE
	};

	hr = d3d.device->CreateRasterizerState( &rastDesc, &d3d.rastState );
	assert( SUCCEEDED( hr ) );

	//=========================================================================
	// Depth Stencil State

	const D3D11_DEPTH_STENCIL_DESC depthStencilDesc{
		.DepthEnable = TRUE,
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
		.DepthFunc = D3D11_COMPARISON_LESS,
	};

	d3d.device->CreateDepthStencilState( &depthStencilDesc, &d3d.depthStencilState );
}

void Shutdown()
{
	d3d.depthStencilState->Release();
	d3d.rastState->Release();

	d3d.inputLayout->Release();
	d3d.pixelShader->Release();
	d3d.vertexShader->Release();
	d3d.constantBuffer->Release();

	d3d.depthStencilView->Release();
	d3d.renderTargetView->Release();

	d3d.swapChain->Release();
	d3d.immediateContext->Release();
	d3d.device->Release();
	d3d.factory->Release();
}

} // namespace Renderer
