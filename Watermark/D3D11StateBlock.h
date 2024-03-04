#pragma once
#include <d3d11.h>

class D3D11StateBlock{
public:
	D3D11StateBlock(ID3D11DeviceContext* pContext);
	~D3D11StateBlock();

	void Store();
	void Restore();
	

private:
	void Release();

	ID3D11DeviceContext* m_pContext;
	ID3D11RasterizerState* m_pRasterizerState;

	D3D11_VIEWPORT m_pViewPorts[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX];

	ID3D11RenderTargetView* m_pRenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11DepthStencilView* m_pDepthStencilView;

	D3D11_PRIMITIVE_TOPOLOGY m_pPrimitiveTopology;

	ID3D11Buffer* m_pVertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	UINT m_pStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	UINT m_pOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];


	ID3D11InputLayout* m_pInputLayout;

	ID3D11VertexShader* m_pVertexShader;
	ID3D11PixelShader* m_pPixelShader;
	ID3D11GeometryShader* m_pGeometryShader;

};