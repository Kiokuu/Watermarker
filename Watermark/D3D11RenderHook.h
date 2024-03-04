#pragma once
#include <d3d11.h>

#include "D3D11StateBlock.h"
#include "MinHook.h"


template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

typedef HRESULT(__fastcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__fastcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);


class D3D11RenderHook {
private:
    static D3D11RenderHook* m_pInstance;

public:
    D3D11RenderHook(HWND hWnd);
    ~D3D11RenderHook();

    void Initialize();
    void CreateSwapChain();

    void InitializeD3DResources();

    HRESULT RenderWatermark(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT __fastcall RenderWatermarkStub(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

    HRESULT ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static HRESULT __fastcall ResizeBuffersStub(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    void HookPresent();

private:

    HWND m_hWnd;

    Present_t m_originalPresent;
    ResizeBuffers_t m_originalResizeBuffers;

    D3D11StateBlock* m_pStateBlock;

    IDXGISwapChain* m_pSwapChain;
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pContext;
    ID3D11RenderTargetView* m_pRenderTargetView;
    ID3D11VertexShader* m_pVertexShader;
    ID3D11PixelShader* m_pPixelShader;
    ID3D11InputLayout* m_pInputLayout;
    ID3D11Buffer* m_pVertexBuffer;

    D3D11_VIEWPORT m_vp;

    bool m_bHookInitialized;
};