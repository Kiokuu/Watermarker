#pragma once

#include <d3d11.h>
#include "D3D11StateBlock.h"
#include "MinHook.h"
#include "CImg/CImg.h"

/**
 * @brief Creates a MinHook hook for a function with extended parameters.
 * @tparam T The type of the function pointer.
 * @param pTarget Pointer to the target function.
 * @param pDetour Pointer to the detour function.
 * @param ppOriginal Pointer to store the original function pointer.
 * @return MH_STATUS indicating the status of hook creation.
 */
template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
    return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}

typedef HRESULT(__fastcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags); /**< Type definition for Present function pointer. */
typedef HRESULT(__fastcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags); /**< Type definition for ResizeBuffers function pointer. */

/**
 * @brief Class for hooking Direct3D 11 rendering functions.
 */
class D3D11RenderHook {
private:
    static D3D11RenderHook* m_pInstance; /**< Static pointer to the singleton instance. */

public:
    /**
     * @brief Constructs a D3D11RenderHook object.
     * @param hWnd The handle to the window.
     */
    D3D11RenderHook(HWND hWnd);

    /**
     * @brief Destructor.
     */
    ~D3D11RenderHook();

    /**
     * @brief Initializes the D3D11RenderHook.
     */
    void Initialize();

    /**
     * @brief Creates a Direct3D 11 swap chain.
     */
    void CreateSwapChain();

    /**
     * @brief Initializes Direct3D 11 resources.
     */
    void InitializeD3DResources();

    /**
     * @brief Renders a watermark on the swap chain.
     * @param pSwapChain Pointer to the swap chain.
     * @param SyncInterval The synchronization interval.
     * @param Flags Flags specifying presentation behavior.
     * @return HRESULT indicating the result of rendering.
     */
    HRESULT RenderWatermark(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

    /**
     * @brief Stub function for rendering a watermark.
     * @param pSwapChain Pointer to the swap chain.
     * @param SyncInterval The synchronization interval.
     * @param Flags Flags specifying presentation behavior.
     * @return HRESULT indicating the result of rendering.
     */
    static HRESULT __fastcall RenderWatermarkStub(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

    /**
     * @brief Resizes the swap chain buffers.
     * @param pSwapChain Pointer to the swap chain.
     * @param BufferCount The number of buffers.
     * @param Width The new width of the buffers.
     * @param Height The new height of the buffers.
     * @param NewFormat The new DXGI_FORMAT of the buffers.
     * @param SwapChainFlags Flags specifying swap chain behavior.
     * @return HRESULT indicating the result of resizing.
     */
    HRESULT ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    /**
     * @brief Stub function for resizing the swap chain buffers.
     * @param pSwapChain Pointer to the swap chain.
     * @param BufferCount The number of buffers.
     * @param Width The new width of the buffers.
     * @param Height The new height of the buffers.
     * @param NewFormat The new DXGI_FORMAT of the buffers.
     * @param SwapChainFlags Flags specifying swap chain behavior.
     * @return HRESULT indicating the result of resizing.
     */
    static HRESULT __fastcall ResizeBuffersStub(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    /**
     * @brief Hooks the Present function.
     */
    void HookPresent();

private:
    HWND m_hWnd; /**< Handle to the window. */
    Present_t m_originalPresent; /**< Pointer to the original Present function. */
    ResizeBuffers_t m_originalResizeBuffers; /**< Pointer to the original ResizeBuffers function. */
    D3D11StateBlock* m_pStateBlock; /**< Pointer to the D3D11StateBlock object. */
    IDXGISwapChain* m_pSwapChain; /**< Pointer to the swap chain. */
    ID3D11Device* m_pDevice; /**< Pointer to the D3D11 device. */
    ID3D11DeviceContext* m_pContext; /**< Pointer to the D3D11 device context. */
    ID3D11RenderTargetView* m_pRenderTargetView; /**< Pointer to the render target view. */
    ID3D11VertexShader* m_pVertexShader; /**< Pointer to the vertex shader. */
    ID3D11PixelShader* m_pPixelShader; /**< Pointer to the pixel shader. */
    ID3D11InputLayout* m_pInputLayout; /**< Pointer to the input layout. */
    ID3D11Buffer* m_pIndexBuffer; /**< Pointer to the index buffer. */
    ID3D11Buffer* m_pVertexBuffer; /**< Pointer to the vertex buffer. */
    ID3D11SamplerState* m_pSamplerState; /**< Pointer to the sampler state. */
    ID3D11Texture2D* m_pTexture; /**< Pointer to the texture. */
    ID3D11ShaderResourceView* m_pShaderResourceView; /**< Pointer to the shader resource view. */
    D3D11_VIEWPORT m_vp; /**< Viewport structure. */
    cimg_library::CImg<unsigned char>* m_watermark; /**< Pointer to the watermark image. */
    bool m_bHookInitialized; /**< Flag indicating if the hook is initialized. */
};