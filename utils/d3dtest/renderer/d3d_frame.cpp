
#include "d3d_local.h"

namespace Renderer
{

static const float s_clearColour[4]{ 0.0f, 0.2f, 0.4f, 1.0f };

static StaticCvar d3d_fov( "d3d_fov", "70", 0 );

void Frame()
{
	using namespace DirectX;

	//=========================================================================
	// Mesh

	//model_t &mesh = meshMan.meshes[0];

	//=========================================================================
	// Set shader constants

	D3D11_MAPPED_SUBRESOURCE mappedSubresource;

	d3d.immediateContext->Map( d3d.constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource );

	shaderConstants_t *constants = reinterpret_cast<shaderConstants_t *>( mappedSubresource.pData );

	XMMATRIX workMatrix;

	workMatrix = CL_BuildViewMatrix();
	XMStoreFloat4x4A( &constants->viewMatrix, workMatrix );

	const float fov = d3d_fov.GetFloat();
	const float aspect = wnd_width.GetFloat() / wnd_height.GetFloat();

	workMatrix = XMMatrixPerspectiveFovRH( fov, aspect, 8.0f, 4096.0f );
	XMStoreFloat4x4A( &constants->projMatrix, workMatrix );

	d3d.immediateContext->Unmap( d3d.constantBuffer, 0 );

	// Set stuff up

	const UINT vertexStride = sizeof( meshVertex_t );
	const UINT vertexOffset = 0;

	d3d.immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	d3d.immediateContext->IASetInputLayout( d3d.inputLayout );
	//d3d.immediateContext->IASetVertexBuffers( 0, 1, &mesh.vertexBuffer, &vertexStride, &vertexOffset );
	//d3d.immediateContext->IASetIndexBuffer( mesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0 );

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

	//d3d.immediateContext->DrawIndexed( mesh.numIndices, 0, 0 );

	d3d.immediateContext->OMSetRenderTargets( 0, nullptr, nullptr );

	// End Draw!

	// Present

	d3d.swapChain->Present( 1, 0 );
}

} // namespace Renderer
