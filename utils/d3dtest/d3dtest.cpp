
#include "../../core/core.h"

#include "../../framework/filesystem.h"
#include "../../framework/cmdsystem.h"
#include "../../framework/cvarsystem.h"

#include "../../core/sys_includes.h"
#include <process.h>
#include "../../resources/resource.h"

#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>

#include "../../thirdparty/DirectXMath/Inc/DirectXMath.h"

// Local
#include "meshbuilder.h"
#include "input.h"
#include "window.h"

extern float sys_frameTime;

//=========================================================
// D3D Stuff
//=========================================================

#define MY_FLIP_MODEL DXGI_SWAP_EFFECT_DISCARD // DXGI_SWAP_EFFECT_FLIP_DISCARD
#define SHADER_FILE_NAME "shaders/shader.hlsl"

static const float s_clearColour[4]{ 0.0f, 0.2f, 0.4f, 1.0f };

struct alignas( 16 ) shaderConstants_t
{
	DirectX::XMFLOAT4X4A viewMatrix;
	DirectX::XMFLOAT4X4A projMatrix;
};

static struct d3dDefs_t
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
	ID3D11Buffer *				constantBuffer;
	ID3D11VertexShader *		vertexShader;
	ID3D11PixelShader *			pixelShader;
	ID3D11InputLayout *			inputLayout;
	ID3D11RasterizerState *		rastState;
	ID3D11DepthStencilState *	depthStencilState;
} d3d;

static struct meshDefs_t
{
	TriListMeshBuilder builder;
} mesh;

static StaticCvar d3d_fov( "d3d_fov", "70", 0 );

// Creates our desired factory
static IDXGIFactory7 *D3D_CreateFactory()
{
	HRESULT hr;
	IDXGIFactory7 *factory;

	UINT flags = 0;
#ifdef Q_DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	
	hr = CreateDXGIFactory2( flags, __uuidof( IDXGIFactory7 ), (void **)&factory );
	assert( SUCCEEDED( hr ) );

	return factory;
}

static void D3D_Init()
{
	HRESULT hr;

	// Factory

	d3d.factory = D3D_CreateFactory();

	// Device

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

	// Swap chain

	UINT windowWidth = (UINT)wnd_width.GetInt();
	UINT windowHeight = (UINT)wnd_height.GetInt();
	HWND windowHandle = GetMainWindow();

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

	// Now set up the image buffers

	D3D11_TEXTURE2D_DESC depthBufferDesc; // For later

	ID3D11Texture2D *backBuffer;
	d3d.swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (void **)&backBuffer );

	backBuffer->GetDesc( &depthBufferDesc ); // Save our descriptor

	d3d.device->CreateRenderTargetView( backBuffer, nullptr, &d3d.renderTargetView );
	backBuffer->Release();

	d3d.immediateContext->OMSetRenderTargets( 1, &d3d.renderTargetView, nullptr );

	// Depth Buffer

	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D *depthBuffer;
	d3d.device->CreateTexture2D( &depthBufferDesc, nullptr, &depthBuffer );

	//D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	d3d.device->CreateDepthStencilView( depthBuffer, nullptr, &d3d.depthStencilView );

	// And the viewport

	const D3D11_VIEWPORT viewport
	{
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = static_cast<float>( windowWidth ),
		.Height = static_cast<float>( windowHeight ),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f
	};
	d3d.immediateContext->RSSetViewports( 1, &viewport );

	// Shaders

	D3D11_BUFFER_DESC constantBufferDesc{
		.ByteWidth = sizeof( shaderConstants_t ) + 0xf & 0xfffffff0, // round constant buffer size to 16 byte boundary
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};
	d3d.device->CreateBuffer( &constantBufferDesc, nullptr, &d3d.constantBuffer );

	// Compile shaders

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
	}

	hr = d3d.device->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		nullptr, &d3d.vertexShader );
	assert( SUCCEEDED( hr ) );

	hr = d3d.device->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
		nullptr, &d3d.pixelShader );
	assert( SUCCEEDED( hr ) );

	const D3D11_INPUT_ELEMENT_DESC inputElementDescs[]{
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = d3d.device->CreateInputLayout( inputElementDescs, countof( inputElementDescs ),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &d3d.inputLayout );
	assert( SUCCEEDED( hr ) );

	// Rast State

	D3D11_RASTERIZER_DESC rastDesc{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE
	};

	hr = d3d.device->CreateRasterizerState( &rastDesc, &d3d.rastState );
	assert( SUCCEEDED( hr ) );

	// Depth Stencil State

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{
		.DepthEnable = TRUE,
		.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
		.DepthFunc = D3D11_COMPARISON_LESS,
	};

	d3d.device->CreateDepthStencilState( &depthStencilDesc, &d3d.depthStencilState );

}

static void D3D_Shutdown()
{
	d3d.inputLayout->Release();
	d3d.pixelShader->Release();
	d3d.vertexShader->Release();
	
	d3d.swapChain->Release();

	d3d.renderTargetView->Release();
	d3d.immediateContext->Release();
	d3d.device->Release();

	d3d.factory->Release();
}

static void D3D_Frame()
{
	using namespace DirectX;

#if 0
	const uint numVerts = 32;
	const uint spacing = 400 / numVerts;

	for ( uint i = 0; i <= numVerts; ++i )
	{
		const float x = static_cast<float>( cos( DEG2RAD( static_cast<double>( i * spacing ) ) ) * 100.0 );
		const float y = static_cast<float>( sin( DEG2RAD( static_cast<double>( i * spacing ) ) ) * 100.0 );
		if ( i % 2 == 0 ) {
			mesh.builder.AddVertex( { vec3( 0.0f, 0.0f, 0.0f ) } );
		}
		mesh.builder.AddVertex( { vec3( x, y, 0.0f ) } );
	}
#endif

	vec3 localRed( 1.0f, 0.0f, 0.0f );
	vec3 localGrn( 0.0f, 1.0f, 0.0f );
	vec3 localBlu( 0.0f, 0.0f, 1.0f );

	mesh.builder.AddVertex( { vec3( 32.0f, 0.0f, 0.0f ), localRed } );
	mesh.builder.AddVertex( { vec3( 8.0f, -16.0f, 0.0f ), localGrn } );
	mesh.builder.AddVertex( { vec3( 8.0f, 16.0f, 0.0f ), localBlu } );

	const UINT vertexStride = sizeof( simpleVertex_t );
	const UINT vertexOffset = 0;
	const UINT vertexCount = (UINT)mesh.builder.GetNumVertices();

	const D3D11_BUFFER_DESC desc{
		.ByteWidth = (UINT)mesh.builder.GetSizeInBytes(),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = 0,
		.MiscFlags = 0,
		.StructureByteStride = 0
	};

	const D3D11_SUBRESOURCE_DATA srData{
		.pSysMem = mesh.builder.GetData(),
		.SysMemPitch = 0,
		.SysMemSlicePitch = 0
	};

	ID3D11Buffer *triangleBuffer;

	HRESULT hr = d3d.device->CreateBuffer( &desc, &srData, &triangleBuffer );
	assert( SUCCEEDED( hr ) );

	mesh.builder.Clear();

	D3D11_MAPPED_SUBRESOURCE mappedSubresource;

	d3d.immediateContext->Map( d3d.constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource );

	shaderConstants_t *constants = reinterpret_cast<shaderConstants_t *>( mappedSubresource.pData );

	XMMATRIX workMatrix;

	workMatrix = GetViewMatrix();
	XMStoreFloat4x4A( &constants->viewMatrix, workMatrix );

	float fov = d3d_fov.GetFloat();
	float width = wnd_width.GetFloat();
	float height = wnd_height.GetFloat();

	workMatrix = XMMatrixPerspectiveFovRH( fov, width / height, 8.0f, 4096.0f );
	XMStoreFloat4x4A( &constants->projMatrix, workMatrix );

	d3d.immediateContext->Unmap( d3d.constantBuffer, 0 );

	// Set stuff up

	d3d.immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
	d3d.immediateContext->IASetInputLayout( d3d.inputLayout );
	d3d.immediateContext->IASetVertexBuffers( 0, 1, &triangleBuffer, &vertexStride, &vertexOffset );

	d3d.immediateContext->RSSetState( d3d.rastState );

	d3d.immediateContext->VSSetShader( d3d.vertexShader, nullptr, 0 );
	d3d.immediateContext->VSSetConstantBuffers( 0, 1, &d3d.constantBuffer );

	d3d.immediateContext->PSSetShader( d3d.pixelShader, nullptr, 0 );

	// Clear view

	d3d.immediateContext->ClearRenderTargetView( d3d.renderTargetView, s_clearColour );
	d3d.immediateContext->ClearDepthStencilView( d3d.depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	// Start Draw!

	d3d.immediateContext->OMSetRenderTargets( 1, &d3d.renderTargetView, nullptr );
	d3d.immediateContext->OMSetDepthStencilState( d3d.depthStencilState, 0 );
	d3d.immediateContext->OMSetBlendState( nullptr, nullptr, UINT_MAX ); // use default blend mode (i.e. disable)

	d3d.immediateContext->Draw( vertexCount, 0 );

	d3d.immediateContext->OMSetRenderTargets( 0, nullptr, nullptr );

	// End Draw!

	triangleBuffer->Release();

	// Present

	d3d.swapChain->Present( 1, 0 );
}

//=========================================================
// Main
//=========================================================

static void Com_Init( int argc, char **argv )
{
	Time_Init();

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	Cvar_AddEarlyCommands( argc, argv );

	FileSystem::Init();

	Cbuf_AddLateCommands( argc, argv );

	CreateMainWindow( wnd_width.GetInt(), wnd_height.GetInt() );
	Input_Init();

	D3D_Init();
}

static void Com_Shutdown()
{
	D3D_Shutdown();
	DestroyMainWindow();

	FileSystem::Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();
}

static void Com_Quit( int code )
{
	Com_Shutdown();

	exit( code );
}

static void Com_Frame( double deltaTime )
{
	Input_Frame();
	Input_ReportCamera();

	D3D_Frame();
}

int main( int argc, char **argv )
{
#ifdef _WIN32
	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );
#ifdef Q_MEM_DEBUG
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	Com_Init( argc, argv );

	double deltaTime, oldTime, newTime;
	MSG msg;

	oldTime = Time_FloatMilliseconds();

	// Show the main window just before we start doing things
	ShowMainWindow();

	// Main loop
	while ( 1 )
	{
		// Process all queued messages
		while ( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				Com_Quit( EXIT_SUCCESS );
			}
			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}

		newTime = Time_FloatMilliseconds();
		deltaTime = newTime - oldTime;

		sys_frameTime = MS2SEC( deltaTime );

		Com_Frame( MS2SEC( deltaTime ) );

		oldTime = newTime;
	}
	
	// unreachable
	return 0;
}
