#include "D3D11StateBlock.h"

D3D11StateBlock::D3D11StateBlock(ID3D11DeviceContext* pContext) : m_pContext(pContext), m_pRasterizerState(nullptr), m_pDepthStencilView(nullptr), m_pPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
{
}

D3D11StateBlock::~D3D11StateBlock()
{
	Release();

	if (m_pContext)
	{
		m_pContext->Release();
	}
}

void D3D11StateBlock::Store()
{
	m_pContext->RSGetState(&m_pRasterizerState);

	UINT numViewPorts = D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX;
	m_pContext->RSGetViewports(&numViewPorts, m_pViewPorts);
	m_pContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_pRenderTargetViews, &m_pDepthStencilView);
	m_pContext->IAGetPrimitiveTopology(&m_pPrimitiveTopology);
	m_pContext->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, m_pVertexBuffers, m_pStrides, m_pOffsets);
	m_pContext->IAGetInputLayout(&m_pInputLayout);

	m_pContext->GSGetShader(&m_pGeometryShader, nullptr, 0);
	m_pContext->VSGetShader(&m_pVertexShader, nullptr, 0);
	m_pContext->PSGetShader(&m_pPixelShader, nullptr, 0);

}

void D3D11StateBlock::Restore()
{
	m_pContext->RSSetState(m_pRasterizerState);
	m_pContext->RSSetViewports(D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX, m_pViewPorts);
	m_pContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_pRenderTargetViews, m_pDepthStencilView);
	m_pContext->IASetPrimitiveTopology(m_pPrimitiveTopology);
	m_pContext->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, m_pVertexBuffers, m_pStrides, m_pOffsets);
	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->GSSetShader(m_pGeometryShader, nullptr, 0);
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	Release();
}

void D3D11StateBlock::Release()
{
	if (m_pRasterizerState)
	{
		m_pRasterizerState->Release();
		m_pRasterizerState = nullptr;
	}

	if (m_pDepthStencilView)
	{
		m_pDepthStencilView->Release();
		m_pDepthStencilView = nullptr;
	}

	for (auto& m_pRenderTargetView : m_pRenderTargetViews)
	{
		if (m_pRenderTargetView)
		{
			m_pRenderTargetView->Release();
			m_pRenderTargetView = nullptr;
		}
	}

	for (auto& m_pVertexBuffer : m_pVertexBuffers)
	{
		if (m_pVertexBuffer)
		{
			m_pVertexBuffer->Release();
			m_pVertexBuffer = nullptr;
		}
	}

	if (m_pGeometryShader)
	{
		m_pGeometryShader->Release();
		m_pGeometryShader = nullptr;
	}

	if (m_pVertexShader)
	{
		m_pVertexShader->Release();
		m_pVertexShader = nullptr;
	}

	if (m_pPixelShader)
	{
		m_pPixelShader->Release();
		m_pPixelShader = nullptr;
	}
}
