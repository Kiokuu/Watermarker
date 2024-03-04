#include "D3D11RenderHook.h"
#include <d3dcompiler.h>
#include <iostream>

#include "D3D11StateBlock.h"
#include "D3D11WatermarkShader.h"
#include "Vertex.h"


D3D11RenderHook* D3D11RenderHook::m_pInstance = nullptr;

D3D11RenderHook::D3D11RenderHook(HWND hWnd)
    : m_hWnd(hWnd), m_originalPresent(nullptr), m_pSwapChain(nullptr), m_pDevice(nullptr), m_pContext(nullptr),
      m_pRenderTargetView(nullptr), m_pVertexShader(nullptr), m_pPixelShader(nullptr),
      m_pInputLayout(nullptr), m_pVertexBuffer(nullptr), m_bHookInitialized(false)
{
		Initialize();
}

D3D11RenderHook::~D3D11RenderHook() {}


void D3D11RenderHook::Initialize()
{
	if(m_pInstance == nullptr)
	{
		m_pInstance = this;
	}
	else
	{
		std::cerr << "D3D11RenderHook already exists" << '\n';
		return;
	}

	HookPresent();
}

void D3D11RenderHook::InitializeD3DResources()
{
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

	m_pStateBlock = new D3D11StateBlock(m_pContext); 

	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	if(FAILED(hr))
	{
		printf("Failed to create render target view\n");
	}

	pBackBuffer->Release();

	DXGI_SWAP_CHAIN_DESC desc;
	m_pSwapChain->GetDesc(&desc);
	
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_vp.Width = desc.BufferDesc.Width;
	m_vp.Height = desc.BufferDesc.Height;
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;

    // Compile vertex shader
	ID3DBlob* pVertexShaderBlob = nullptr;
    hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVertexShaderBlob, nullptr);
    if (FAILED(hr))
    {
        printf("Failed to compile vertex shader\n");
	}
	
    // Create vertex shader
    hr = m_pDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &m_pVertexShader);
    if (FAILED(hr))
    {
        printf("Failed to create vertex shader\n");
    }

	// Compile pixel shader
	ID3DBlob* pPixelShaderBlob = nullptr;
	hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "ps_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pPixelShaderBlob, nullptr);
    if (FAILED(hr))
    {
	    printf("Failed to compile pixel shader\n");
    }

	// Create pixel shader
	hr = m_pDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	if (FAILED(hr))
	{
		printf("Failed to create pixel shader\n");
	}

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv) , D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    constexpr UINT numElements = ARRAYSIZE(layout);
    hr = m_pDevice->CreateInputLayout(layout, numElements, pVertexShaderBlob->GetBufferPointer(),
                                     pVertexShaderBlob->GetBufferSize(), &m_pInputLayout);
    if (FAILED(hr))
    {
        printf("Failed to create input layout\n");
    }
	
    // Define vertices for a triangle
	Vertex vertices[] =
    {
		{ {-1.0f, -1.0f}, {0.0f, 1.0f}}, // Bottom left
		{ {0.0f, 1.0f},{0.5f, 0.0f }},   // Top center
		{{ 1.0f, -1.0f},{1.0f, 1.0f}}   // Bottom right
    };

    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    hr = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pVertexBuffer);
    if (FAILED(hr))
    {
        printf("Failed to create vertex buffer\n");
    }
}

HRESULT D3D11RenderHook::RenderWatermark(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if(Flags & DXGI_PRESENT_TEST)
	{
		return m_originalPresent(pSwapChain, SyncInterval, Flags);
	}

	if(!m_bHookInitialized)
	{
		m_pSwapChain = pSwapChain;
		InitializeD3DResources();
		m_bHookInitialized = true;
	}


	m_pStateBlock->Store();

	m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);


	// Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
	m_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pContext->IASetInputLayout(m_pInputLayout);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pContext->RSSetViewports(1, &m_vp);

	m_pContext->GSSetShader(nullptr, nullptr, 0);
    m_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader, nullptr, 0);

    // Draw triangle
    m_pContext->Draw(3, 0);

	m_pStateBlock->Restore();

	return m_originalPresent(pSwapChain, SyncInterval, Flags);
}


HRESULT __fastcall D3D11RenderHook::RenderWatermarkStub(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	return m_pInstance->RenderWatermark(pSwapChain, SyncInterval, Flags);
}

HRESULT D3D11RenderHook::ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	m_pRenderTargetView->Release();

	HRESULT hr = m_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
	if(FAILED(hr))
	{
		printf("Failed to resize buffers\n");
		return hr;
	}

	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
    {
        printf("Failed to get back buffer\n");
    	return hr;
    }

	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	if(FAILED(hr))
	{
		printf("Failed to create render target view\n");
		return hr;
	}

	pBackBuffer->Release();

	DXGI_SWAP_CHAIN_DESC desc;
	m_pSwapChain->GetDesc(&desc);
	
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_vp.Width = desc.BufferDesc.Width;
	m_vp.Height = desc.BufferDesc.Height;
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;

	return hr;
}

HRESULT __fastcall D3D11RenderHook::ResizeBuffersStub(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	return m_pInstance->ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

void D3D11RenderHook::HookPresent()
{
	DXGI_SWAP_CHAIN_DESC swapChainDescr = { 0 };
	swapChainDescr.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDescr.BufferDesc.RefreshRate.Denominator = 1; 
	swapChainDescr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
	swapChainDescr.SampleDesc.Count = 1;                               
	swapChainDescr.SampleDesc.Quality = 0;                               
	swapChainDescr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDescr.BufferCount = 1;                               
	swapChainDescr.OutputWindow = m_hWnd;                
	swapChainDescr.Windowed = false;

	IDXGISwapChain* swapChainPtr = nullptr;
	ID3D11Device* devicePtr = nullptr;


	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
				&swapChainDescr, &swapChainPtr, &devicePtr, NULL, NULL)))
	{
		std::cerr << "Failed to create device and swap chain" << '\n';
		return;
	}

	uint64_t* swapChainVtable = *reinterpret_cast<uint64_t**>(swapChainPtr);
	uint64_t* presentPtr = reinterpret_cast<uint64_t*>(swapChainVtable[8]);
	uint64_t* resizeBufferPtr = reinterpret_cast<uint64_t*>(swapChainVtable[13]);

	if (MH_Initialize() != MH_OK)
	{
		std::cerr << "Failed to initialize MinHook" << '\n';
		return;
	}

	if(MH_CreateHookEx(presentPtr, &RenderWatermarkStub, &m_originalPresent) != MH_OK)
	{
		std::cerr << "Failed to create hook" << '\n';
		return;
	}

	if(MH_CreateHookEx(resizeBufferPtr, &ResizeBuffersStub, &m_originalResizeBuffers) != MH_OK)
	{
		std::cerr << "Failed to create hook" << '\n';
		return;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		std::cerr << "Failed to enable hooks" << '\n';
		return;
	}

	//swapChainPtr->Release();
	//devicePtr->Release();
}

