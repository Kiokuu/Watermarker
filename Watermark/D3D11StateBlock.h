#pragma once

#include <d3d11.h>

/**
 * @brief Class representing a state block in Direct3D 11.
 */
class D3D11StateBlock
{
public:
	/**
	 * @brief Constructor.
	 * @param pContext The device context associated with the state block.
	 */
	D3D11StateBlock(ID3D11DeviceContext* pContext);

	/**
	 * @brief Destructor.
	 */
	~D3D11StateBlock();

	/**
	 * @brief Stores the current device context state.
	 */
	void Store();

	/**
	 * @brief Restores the device context state to the stored state.
	 */
	void Restore();

private:
	/**
	 * @brief Releases allocated resources.
	 */
	void Release();

	ID3D11DeviceContext* m_pContext; /**< Pointer to the device context. */
	ID3D11RasterizerState* m_pRasterizerState; /**< Rasterizer state. */

	D3D11_VIEWPORT m_pViewPorts[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX]; /**< Array of viewports. */

	ID3D11RenderTargetView* m_pRenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	/**< Array of render target views. */
	ID3D11DepthStencilView* m_pDepthStencilView; /**< Depth stencil view. */

	D3D11_PRIMITIVE_TOPOLOGY m_pPrimitiveTopology; /**< Primitive topology. */

	ID3D11Buffer* m_pVertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]; /**< Array of vertex buffers. */
	ID3D11Buffer* m_pIndexBuffer; /**< Index buffer. */

	DXGI_FORMAT m_pIndexBufferFormat; /**< Format of the index buffer. */
	UINT m_uiOffset; /**< Offset for the index buffer. */

	UINT m_pStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]; /**< Array of buffer strides. */
	UINT m_pOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]; /**< Array of buffer offsets. */

	ID3D11InputLayout* m_pInputLayout; /**< Input layout. */

	ID3D11VertexShader* m_pVertexShader; /**< Vertex shader. */
	ID3D11PixelShader* m_pPixelShader; /**< Pixel shader. */
	ID3D11GeometryShader* m_pGeometryShader; /**< Geometry shader. */

	ID3D11ShaderResourceView* m_pShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	/**< Array of shader resource views. */

	ID3D11BlendState* m_pBlendState; /**< Blend state. */
	UINT m_blendSampleMask; /**< Blend sample mask. */
	float m_blendFactor[4]; /**< Blend factors. */

	ID3D11SamplerState* m_pSamplerStates[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]; /**< Array of sampler states. */
};
