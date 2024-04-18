#include "D3D11RenderHook.h"
#include <d3dcompiler.h>
#include <iostream>

#include <vector>

#include "CImg/CImg.h"

#include "D3D11StateBlock.h"
#include "D3D11WatermarkShader.h"
#include "Vertex.h"


D3D11RenderHook* D3D11RenderHook::m_pInstance = nullptr;

D3D11RenderHook::D3D11RenderHook(HWND hWnd, const char* watermark_data)
	: m_hWnd(hWnd), m_watermarkData(watermark_data), m_originalPresent(nullptr), m_pSwapChain(nullptr),
	  m_pDevice(nullptr), m_pContext(nullptr),
	  m_pRenderTargetView(nullptr), m_pVertexShader(nullptr), m_pPixelShader(nullptr),
	  m_pInputLayout(nullptr), m_pVertexBuffer(nullptr), m_pSamplerState(nullptr), m_pTexture(nullptr),
	  m_bHookInitialized(false)
{
	// Initialize the render hook
	Initialize();
}

D3D11RenderHook::~D3D11RenderHook()
{
}


void D3D11RenderHook::Initialize()
{
	// Check if already initialized
	if (m_pInstance == nullptr)
	{
		m_pInstance = this;
	}
	else
	{
		std::cerr << "D3D11RenderHook already exists" << '\n';
		return;
	}

	// Hook the present function
	Hook();
}

void D3D11RenderHook::InitializeD3DResources()
{
	// Get the back buffer
	ID3D11Texture2D* pBackBuffer = nullptr;
	HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
	{
		printf("Failed to get back buffer\n");
	}

	// Get the device
	hr = m_pSwapChain->GetDevice(__uuidof(ID3D11Device), (LPVOID*)&m_pDevice);
	if (FAILED(hr))
	{
		printf("Failed to get device\n");
	}

	// Get the device context
	m_pDevice->GetImmediateContext(&m_pContext);
	if (!m_pContext)
	{
		printf("Failed to get device context\n");
	}

	// Create a stateblock to store the current state and restore it after rendering
	m_pStateBlock = new D3D11StateBlock(m_pContext);

	// Create a render target view
	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	if (FAILED(hr))
	{
		printf("Failed to create render target view\n");
	}

	// Release the backbuffer as it is no longer needed
	pBackBuffer->Release();

	// Get the sawpchain descriptor
	DXGI_SWAP_CHAIN_DESC desc;
	m_pSwapChain->GetDesc(&desc);

	// Get the watermark text as a const char*
	auto watermarkText = m_watermarkData.c_str();
	int width = desc.BufferDesc.Width;
	int height = desc.BufferDesc.Height;

	int fontHeight = 13 * (height / 760.0f);

	// Create a transparent background image
	cimg_library::CImg<unsigned char> watermark(width, height, 1, 4, 0);

	constexpr unsigned char white[] = {255, 255, 255, 255};

	// Calculate the number of repetitions of the text in the image
	int numRepetitionsX = (width + 100) / 100;
	int numRepetitionsY = (height + 20) / 20;

	// Draw the text at each repetition position
	for (int y = 0; y < numRepetitionsY; ++y)
	{
		for (int x = 0; x < numRepetitionsX; ++x)
		{
			watermark.draw_text(x * 100, y * 20, watermarkText, white, 0, 1, fontHeight);
		}
	}

	//watermark.rotate(45); // Rotate the texture by 45 degrees, need to make the image bigger for this to work........

	// Get the pixel data of the image
	unsigned char* pixels = watermark.data();

	// Interleave the pixels to be in the format R, G, B, A, R, G, B, A, ...
	std::vector<unsigned char> interleavedPixels(width * height * 4);
	for (int i = 0; i < width * height; ++i)
	{
		interleavedPixels[i * 4] = pixels[i]; // R channel
		interleavedPixels[i * 4 + 1] = pixels[i + width * height]; // G channel
		interleavedPixels[i * 4 + 2] = pixels[i + 2 * width * height]; // B channel

		// Set the alpha channel to 255 if the pixel is not black
		interleavedPixels[i * 4 + 3] = pixels[i] != 0 || pixels[i + width * height] != 0 || pixels[i + 2 * width *
			                               height] != 0
			                               ? 255
			                               : 0;
	}


	// Save the image to a file (for debugging)
	watermark.save_bmp("watermark.bmp");


	// Create a texture2d descriptor
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create a subresource data descriptor initialized with the interleaved pixels
	D3D11_SUBRESOURCE_DATA initDataTex = {};
	initDataTex.pSysMem = interleavedPixels.data();
	initDataTex.SysMemPitch = width * 4;

	// Create the texture
	hr = m_pDevice->CreateTexture2D(&textureDesc, &initDataTex, &m_pTexture);
	if (FAILED(hr))
	{
		printf("Failed to create texture\n");
	}

	// Create a blend descriptor
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;

	// Set the blend state to blend the watermark with the background
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	// Create the blendstate.
	hr = m_pDevice->CreateBlendState(&blendDesc, &m_pBlendState);
	if (FAILED(hr))
	{
		printf("Failed to create blend state\n");
	}

	// Create a shader resource view.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = m_pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, &m_pShaderResourceView);
	if (FAILED(hr))
	{
		printf("Failed to create shader resource view\n");
	}

	// Create a sampler, to sample the texture.
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerState);
	if (FAILED(hr))
	{
		printf("Failed to create sampler state\n");
	}

	// Set the viewport
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_vp.Width = desc.BufferDesc.Width;
	m_vp.Height = desc.BufferDesc.Height;
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;

	// Compile vertex shader
	ID3DBlob* pVertexShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "vs_main", "vs_5_0",
	                D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVertexShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		printf("Failed to compile vertex shader\n");
		printf("%s\n", static_cast<char*>(pErrorBlob->GetBufferPointer()));
	}

	// Create vertex shader
	hr = m_pDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(),
	                                   nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		printf("Failed to create vertex shader\n");
	}

	// Compile pixel shader
	ID3DBlob* pPixelShaderBlob = nullptr;
	hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "ps_main", "ps_5_0",
	                D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPixelShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		printf("Failed to compile pixel shader\n");
		printf("%s\n", static_cast<char*>(pErrorBlob->GetBufferPointer()));
	}

	// Create pixel shader
	hr = m_pDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr,
	                                  &m_pPixelShader);
	if (FAILED(hr))
	{
		printf("Failed to create pixel shader\n");
	}

	// Create input layout, based on vertex.
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) + sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	constexpr UINT numElements = ARRAYSIZE(layout);
	hr = m_pDevice->CreateInputLayout(layout, numElements, pVertexShaderBlob->GetBufferPointer(),
	                                  pVertexShaderBlob->GetBufferSize(), &m_pInputLayout);
	if (FAILED(hr))
	{
		printf("Failed to create input layout\n");
	}

	// Create index buffer
	UINT indexBuffer[] = {0, 1, 2, 3};

	// Create vertex buffer
	Vertex vertices[] = {
		{{0, 0}, {0, 0}}, // Bottom left
		{{1, 0}, {1, 0}}, // Bottom right
		{{0, 1}, {0, 1}}, // Top left
		{{1, 1}, {1, 1}} // Top right
	};

	// Create index buffer descriptor for the quad.
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(indexBuffer);
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;

	// Create buffer with the index data
	D3D11_SUBRESOURCE_DATA initDataIndex = {};
	initDataIndex.pSysMem = indexBuffer;

	hr = m_pDevice->CreateBuffer(&indexBufferDesc, &initDataIndex, &m_pIndexBuffer);
	if (FAILED(hr))
	{
		printf("Failed to create index buffer\n");
	}

	// Create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;

	// Initialize the vertex buffer with the vertices
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;

	// Create the vertex buffer
	hr = m_pDevice->CreateBuffer(&vertexBufferDesc, &initData, &m_pVertexBuffer);
	if (FAILED(hr))
	{
		printf("Failed to create vertex buffer\n");
	}
}

HRESULT D3D11RenderHook::RenderWatermark(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	// If the test flag is set, return immediately
	if (Flags & DXGI_PRESENT_TEST)
	{
		return m_originalPresent(pSwapChain, SyncInterval, Flags);
	}

	// If the hook data is not initialized, initialize it
	if (!m_bHookInitialized)
	{
		m_pSwapChain = pSwapChain;
		InitializeD3DResources();
		m_bHookInitialized = true;
	}

	// Store the current state
	m_pStateBlock->Store();

	// Set the render target view and blend state
	m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);
	m_pContext->OMSetBlendState(m_pBlendState, nullptr, 0xffffffff);

	// Set vertex and index buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	m_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the input layout and topology
	m_pContext->IASetInputLayout(m_pInputLayout);
	m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the viewport
	m_pContext->RSSetViewports(1, &m_vp);

	// Set the pixel shader sampler and resources
	m_pContext->PSSetSamplers(0, 1, &m_pSamplerState);
	m_pContext->PSSetShaderResources(0, 1, &m_pShaderResourceView);

	// Set the shaders
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

	// Draw the watermark
	m_pContext->DrawIndexed(4, 0, 0);

	// Restore the state after drawing
	m_pStateBlock->Restore();

	return m_originalPresent(pSwapChain, SyncInterval, Flags);
}


HRESULT __fastcall D3D11RenderHook::RenderWatermarkStub(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	return m_pInstance->RenderWatermark(pSwapChain, SyncInterval, Flags);
}

HRESULT D3D11RenderHook::ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height,
                                       DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	// Print the new width and height.
	printf("Resizing buffers to %d x %d\n", Width, Height);

	// Release the render target view
	m_pRenderTargetView->Release();

	// Call the original resize buffers function to utilize the changes
	HRESULT hr = m_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
	if (FAILED(hr))
	{
		printf("Failed to resize buffers\n");
		return hr;
	}

	// Get the back buffer
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
	{
		printf("Failed to get back buffer\n");
		return hr;
	}

	// Create a render target view based on the new size
	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	if (FAILED(hr))
	{
		printf("Failed to create render target view\n");
		return hr;
	}

	// Release the back buffer
	pBackBuffer->Release();

	// Get the swapchain descriptor for the new size
	DXGI_SWAP_CHAIN_DESC desc;
	m_pSwapChain->GetDesc(&desc);

	// Set the viewport based on the new size
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_vp.Width = desc.BufferDesc.Width;
	m_vp.Height = desc.BufferDesc.Height;
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;

	return hr;
}

HRESULT __fastcall D3D11RenderHook::ResizeBuffersStub(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width,
                                                      UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	return m_pInstance->ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

void D3D11RenderHook::Hook()
{
	// Create a swap chain descriptor attached to the window
	DXGI_SWAP_CHAIN_DESC swapChainDescr = {0};
	swapChainDescr.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDescr.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDescr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDescr.SampleDesc.Count = 1;
	swapChainDescr.SampleDesc.Quality = 0;
	swapChainDescr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDescr.BufferCount = 1;
	swapChainDescr.OutputWindow = m_hWnd;
	swapChainDescr.Windowed = false;

	// Store the pointers to the swap chain and device
	IDXGISwapChain* swapChainPtr = nullptr;
	ID3D11Device* devicePtr = nullptr;

	// Create swapchain and device, and store the swapchain and device pointers
	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
		&swapChainDescr, &swapChainPtr, &devicePtr, NULL, NULL)))
	{
		std::cerr << "Failed to create device and swap chain" << '\n';
		return;
	}

	// Get the vtable of the swap chain
	uint64_t* swapChainVtable = *reinterpret_cast<uint64_t**>(swapChainPtr);

	// Get the present pointer within the vtable
	auto presentPtr = reinterpret_cast<uint64_t*>(swapChainVtable[8]);

	// Get the resize buffer pointer within the vtable
	auto resizeBufferPtr = reinterpret_cast<uint64_t*>(swapChainVtable[13]);

	// Initialize MinHook
	if (MH_Initialize() != MH_OK)
	{
		std::cerr << "Failed to initialize MinHook" << '\n';
		return;
	}

	// Create a hook for the present function
	if (MH_CreateHookEx(presentPtr, &RenderWatermarkStub, &m_originalPresent) != MH_OK)
	{
		std::cerr << "Failed to create present hook" << '\n';
		return;
	}

	// Create a hook for the resize buffers function
	if (MH_CreateHookEx(resizeBufferPtr, &ResizeBuffersStub, &m_originalResizeBuffers) != MH_OK)
	{
		std::cerr << "Failed to create resize buffer hook" << '\n';
		return;
	}

	// Enable the hooks
	if (MH_EnableHook(nullptr) != MH_OK)
	{
		std::cerr << "Failed to enable hooks" << '\n';
	}

	// Release the swap chain and device pointers
	//swapChainPtr->Release();
	//devicePtr->Release();
}
