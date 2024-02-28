#include "Watermark.h"
#include <iostream>
#include <Windows.h>
#include "MinHook.h"
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

typedef HRESULT(__fastcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present_t oPresent = nullptr;

bool __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return true;
}

struct Vertex
{
    float x, y, z;
	float u, v;
};


const char* vertexShaderSource = R"(
	struct vs_in {
	    float3 position_local : POSITION;
		float2 uv : TEXCOORD0;
	};

	struct vs_out {
	    float4 position_clip : SV_POSITION;
		float2 uv : TEXCOORD0;
	};

	vs_out vs_main(vs_in input) {
	  vs_out output = (vs_out)0;
	  output.position_clip = float4(input.position_local, 1);
	  output.uv = input.uv;
	  return output;
	}

	float4 ps_main(vs_out input) : SV_TARGET {
	  return float4(input.uv, 0, 0.5);
	}
)";

IDXGISwapChain* pSwapChainPtr = nullptr;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;

ID3D11VertexShader* pVertexShader = nullptr;
ID3D11PixelShader* pPixelShader = nullptr;
ID3D11InputLayout* pInputLayout = nullptr;
ID3D11Buffer* pVertexBuffer = nullptr;


D3D11_VIEWPORT vp;


void InitializeD3DResources()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = pSwapChainPtr->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
    {
        printf("Failed to get back buffer\n");
    }

    // Get the device
    hr = pSwapChainPtr->GetDevice(__uuidof(ID3D11Device), (LPVOID*)&pDevice);
    if (FAILED(hr))
    {
        printf("Failed to get device\n");
    }

    // Get the device context
    pDevice->GetImmediateContext(&pContext);
    if (!pContext)
    {
        printf("Failed to get device context\n");
    }

	hr = pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
	if(FAILED(hr))
	{
		printf("Failed to create render target view\n");
	}


	DXGI_SWAP_CHAIN_DESC desc;
	pSwapChainPtr->GetDesc(&desc);
	
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = desc.BufferDesc.Width;
	vp.Height = desc.BufferDesc.Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

    // Compile vertex shader
	ID3DBlob* pVertexShaderBlob = nullptr;
    hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pVertexShaderBlob, nullptr);
    if (FAILED(hr))
    {
        printf("Failed to compile vertex shader\n");
	}
	
    // Create vertex shader
    hr = pDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &pVertexShader);
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
	hr = pDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &pPixelShader);
	if (FAILED(hr))
	{
		printf("Failed to create pixel shader\n");
	}

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    constexpr UINT numElements = ARRAYSIZE(layout);
    hr = pDevice->CreateInputLayout(layout, numElements, pVertexShaderBlob->GetBufferPointer(),
                                     pVertexShaderBlob->GetBufferSize(), &pInputLayout);
    if (FAILED(hr))
    {
        printf("Failed to create input layout\n");
    }
	
    // Define vertices for a triangle
	Vertex vertices[] =
    {
        { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f}, // Bottom left
        { 0.0f, 1.0f, 0.0f, 0.5f, 0.0f },   // Top center
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f}   // Bottom right
    };

    // Create vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    hr = pDevice->CreateBuffer(&bufferDesc, &initData, &pVertexBuffer);
    if (FAILED(hr))
    {
        printf("Failed to create vertex buffer\n");
    }
}



HRESULT __fastcall PresentDetour(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	printf("Present Called\n");

	// Initialize D3D resources if they haven't been initialized yet
	if(!pSwapChainPtr)
	{
		pSwapChainPtr = pSwapChain;
		InitializeD3DResources();
	}

	
	pContext->RSSetViewports(1, &vp);
	pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

    pContext->VSSetShader(pVertexShader, nullptr, 0);
	pContext->PSSetShader(pPixelShader, nullptr, 0);

    pContext->IASetInputLayout(pInputLayout);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    // Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);

    // Draw triangle
    pContext->Draw(3, 0);

    return oPresent(pSwapChain, SyncInterval, Flags);
}

extern "C" __declspec(dllexport) void Watermark()
{
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	//MessageBoxA(0, "Boo!", "Title", 0);

	/*
	 * WINDOW CREATION
	*/

	WNDCLASS wnd;
	wnd.style         = CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc   = DefWindowProc;
	wnd.cbClsExtra    = 0;
	wnd.cbWndExtra    = 0;
	wnd.hInstance     = GetModuleHandle(NULL);
	wnd.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wnd.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wnd.lpszMenuName  = NULL;
	wnd.lpszClassName = "Watermark";

	if(!RegisterClass(&wnd))
	{
		MessageBoxA(0, "Failed to register window class", "Error", 0);
		return;
	}

	HWND hWnd = CreateWindow("Watermark", "Watermark", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, NULL, NULL);
	if(hWnd == NULL)
	{
		MessageBoxA(0, "Failed to create window", "Error", 0);
		return;
	}

	/*
	 * D3D11 INITIALIZATION
	*/
	ID3D11Device* device_ptr                       = NULL;
	IDXGISwapChain* swap_chain_ptr                 = NULL;

	DXGI_SWAP_CHAIN_DESC swap_chain_descr               = { 0 };
	swap_chain_descr.BufferDesc.RefreshRate.Numerator   = 0;
	swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1; 
	swap_chain_descr.BufferDesc.Format  = DXGI_FORMAT_B8G8R8A8_UNORM; 
	swap_chain_descr.SampleDesc.Count   = 1;                               
	swap_chain_descr.SampleDesc.Quality = 0;                               
	swap_chain_descr.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_descr.BufferCount        = 1;                               
	swap_chain_descr.OutputWindow       = hWnd;                
	swap_chain_descr.Windowed           = false;

	if(FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
		&swap_chain_descr, &swap_chain_ptr, &device_ptr, NULL, NULL)))
	{
		MessageBoxA(0, "Failed to create device and swap chain", "Error", 0);
		return;
	}

	/*
	 * HOOKING
	*/
	uint64_t* swap_chain_vtable = *reinterpret_cast<uint64_t**>(swap_chain_ptr);
	uint64_t* present_ptr = reinterpret_cast<uint64_t*>(swap_chain_vtable[8]);

	if(MH_Initialize() != MH_OK)
	{
		MessageBoxA(0, "Failed to initialize MinHook", "Error", 0);
		return;
	}

	MH_CreateHookEx(present_ptr, &PresentDetour, &oPresent);

	if(MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		MessageBoxA(0, "Failed to enable hooks", "Error", 0);
		return;
	}

	/*
	 * CLEANUP
	*/
	//swap_chain_ptr->Release();
	device_ptr->Release();
	

	DestroyWindow(hWnd);
	UnregisterClass("Watermark", GetModuleHandle(NULL));


}


