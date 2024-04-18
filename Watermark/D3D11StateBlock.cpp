/*
 * This file contains a modified version of the D11StateBlock.cpp source file from the Mumble project,
 * which is licensed under the BSD-3 License. The original copyright notice and license text are
 * retained below:
 */

// Original copyright notice:
// Copyright 2014-2023 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.
//
// Copyright (C) 2011-2013, Nye Liu <nyet@nyet.org>
// Copyright (C) 2011-2013, Kissaki <kissaki@gmx.de>
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// - Neither the name of the Mumble Developers nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "D3D11StateBlock.h"

D3D11StateBlock::D3D11StateBlock(ID3D11DeviceContext* pContext) : m_pContext(pContext)
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
	m_pContext->OMGetBlendState(&m_pBlendState, m_blendFactor, &m_blendSampleMask);

	m_pContext->IAGetPrimitiveTopology(&m_pPrimitiveTopology);
	m_pContext->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, m_pVertexBuffers, m_pStrides,
	                               m_pOffsets);

	m_pContext->IAGetIndexBuffer(&m_pIndexBuffer, &m_pIndexBufferFormat, &m_uiOffset);

	m_pContext->IAGetInputLayout(&m_pInputLayout);

	m_pContext->GSGetShader(&m_pGeometryShader, nullptr, nullptr);
	m_pContext->VSGetShader(&m_pVertexShader, nullptr, nullptr);
	m_pContext->PSGetShader(&m_pPixelShader, nullptr, nullptr);

	m_pContext->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, m_pSamplerStates);

	m_pContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, m_pShaderResourceViews);
}

void D3D11StateBlock::Restore()
{
	m_pContext->RSSetState(m_pRasterizerState);
	m_pContext->RSSetViewports(D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX, m_pViewPorts);
	m_pContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, m_pRenderTargetViews, m_pDepthStencilView);
	m_pContext->OMSetBlendState(m_pBlendState, m_blendFactor, m_blendSampleMask);

	m_pContext->IASetPrimitiveTopology(m_pPrimitiveTopology);
	m_pContext->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, m_pVertexBuffers, m_pStrides,
	                               m_pOffsets);

	m_pContext->IASetIndexBuffer(m_pIndexBuffer, m_pIndexBufferFormat, m_uiOffset);

	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->GSSetShader(m_pGeometryShader, nullptr, 0);
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	m_pContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, m_pSamplerStates);

	m_pContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, m_pShaderResourceViews);
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

	if (m_pInputLayout)
	{
		m_pInputLayout->Release();
		m_pInputLayout = nullptr;
	}

	if (m_pIndexBuffer)
	{
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
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

	for (auto& m_pSamplerState : m_pSamplerStates)
	{
		if (m_pSamplerState)
		{
			m_pSamplerState->Release();
			m_pSamplerState = nullptr;
		}
	}

	for (auto& m_pShaderResourceView : m_pShaderResourceViews)
	{
		if (m_pShaderResourceView)
		{
			m_pShaderResourceView->Release();
			m_pShaderResourceView = nullptr;
		}
	}

	if (m_pBlendState)
	{
		m_pBlendState->Release();
		m_pBlendState = nullptr;
	}
}
