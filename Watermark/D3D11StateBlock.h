/*
 * This file contains a modified version of the D11StateBlock.h header file from the Mumble project,
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
