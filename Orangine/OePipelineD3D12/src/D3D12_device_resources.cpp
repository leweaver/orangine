//
// D3D12_device_resources.cpp - A wrapper for the Direct3D 11 device and swapchain
//                       (requires DirectX 11.1 Runtime)
//

#include "D3D12_device_resources.h"
#include "OeCore/EngineUtils.h"

#include <wrl/client.h>

using namespace DirectX;
using namespace oe;

using Microsoft::WRL::ComPtr;

// Constructor for D3D12_device_resources.
D3D12_device_resources::D3D12_device_resources(
    HWND hwnd,
    DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    UINT backBufferCount,
    D3D_FEATURE_LEVEL minFeatureLevel,
    unsigned int flags)
    : m_screenViewport{}
    , m_backBufferFormat(backBufferFormat)
    , m_depthBufferFormat(depthBufferFormat)
    , m_backBufferCount(backBufferCount)
    , m_d3dMinFeatureLevel(minFeatureLevel)
    , m_window(hwnd)
    , m_d3dFeatureLevel(D3D_FEATURE_LEVEL_12_1)
    , m_outputSize{0, 0, 1, 1}
    , m_colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
    , m_options(flags)
    , m_deviceNotify(nullptr)
    , m_outputDetailedMemoryReport(true) {}

// Configures the Direct3D device, and stores handles to it and the device context.
void D3D12_device_resources::createDeviceDependentResources() {
#if defined(_DEBUG)
  // Enable the D3D12 debug layer.
  {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
      debugController->EnableDebugLayer();
    } else {
      OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
    }
  }
#endif

  CreateFactory();

  // Determines whether tearing support is available for fullscreen borderless windows.
  if (m_options & c_AllowTearing) {
    BOOL allowTearing = FALSE;

    ComPtr<IDXGIFactory5> factory5;
    HRESULT hr = m_dxgiFactory.As(&factory5);
    if (SUCCEEDED(hr)) {
      hr = factory5->CheckFeatureSupport(
          DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
    }

    if (FAILED(hr) || !allowTearing) {
      m_options &= ~c_AllowTearing;
#ifdef _DEBUG
      OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
    }
  }

  // Disable HDR if we are on an OS that can't support FLIP swap effects
  if (m_options & c_EnableHDR) {
    ComPtr<IDXGIFactory5> factory5;
    if (FAILED(m_dxgiFactory.As(&factory5))) {
      m_options &= ~c_EnableHDR;
#ifdef _DEBUG
      OutputDebugStringA("WARNING: Flip swap effects not supported");
#endif
    }
  }

  // Determine DirectX hardware feature levels this app will support.
  static const D3D_FEATURE_LEVEL s_featureLevels[] = {
      D3D_FEATURE_LEVEL_12_1,
  };

  UINT featLevelCount = 0;
  for (; featLevelCount < _countof(s_featureLevels); ++featLevelCount) {
    if (s_featureLevels[featLevelCount] < m_d3dMinFeatureLevel)
      break;
  }

  if (!featLevelCount) {
    OE_THROW(std::out_of_range("minFeatureLevel too high"));
  }

  ComPtr<IDXGIAdapter1> adapter;
  GetHardwareAdapter(adapter.GetAddressOf());

  // Create the Direct3D 11 API device object and a corresponding context.
  ComPtr<ID3D12Device> device;

  HRESULT hr = E_FAIL;
  if (adapter) {
    for (auto featureLevel : s_featureLevels) {
      hr = D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&device));
      if (SUCCEEDED(hr)) {
        break;
      }
    }
  }
#if defined(NDEBUG)
  else {
    OE_THROW(std::exception("No Direct3D hardware device found"));
  }
#else
  if (FAILED(hr)) {
    // If the initialization fails, fall back to the WARP device.
    // For more information on WARP, see:
    // http://go.microsoft.com/fwlink/?LinkId=286690

    ComPtr<IDXGIAdapter> warpAdapter;
    ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

    hr = D3D12CreateDevice(warpAdapter.Get(), s_featureLevels[0], IID_PPV_ARGS(&device));

    if (SUCCEEDED(hr)) {
      OutputDebugStringA("Direct3D Adapter - WARP\n");
    }
  }
#endif

  ThrowIfFailed(hr);

#ifndef NDEBUG
  ComPtr<ID3D12DebugDevice> d3dDebugDevice;
  if (SUCCEEDED(device.As(&d3dDebugDevice))) {
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(d3dDebugDevice.As(&d3dInfoQueue))) {
#ifdef _DEBUG
      d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
      d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
      D3D12_MESSAGE_ID hide[] = {
          D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
      };
      D3D12_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = _countof(hide);
      filter.DenyList.pIDList = hide;
      d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
  }
#endif

  ThrowIfFailed(device.As(&m_d3dDevice));

  createRenderPipeline(_renderPipeline);
}

void D3D12_device_resources::destroyDeviceDependentResources() {
  _renderPipeline = {};

#ifdef _DEBUG
  if (m_outputDetailedMemoryReport) {
    ComPtr<ID3D12DebugDevice1> d3dDebugDevice;
    if (m_d3dDevice && SUCCEEDED(m_d3dDevice->QueryInterface<ID3D12DebugDevice1>(&d3dDebugDevice))) {
      m_d3dDevice.Reset();
      d3dDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL);
    } else {
      OutputDebugStringA("Failed to get d3dDebugDevice");
    }
  }
#endif

  m_d3dDevice.Reset();
}

// These resources need to be recreated every time the window size is changed.
void D3D12_device_resources::recreateWindowSizeDependentResources() {
  if (!m_window) {
    OE_THROW(std::exception("Call SetWindow with a valid Win32 window handle"));
  }

  GetClientRect(m_window, &m_outputSize);
  // Clear the previous window size specific context.
  /*LWL
  ID3D11RenderTargetView* nullViews[] = {nullptr};
  m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
  m_d3dRenderTargetView.Reset();
  m_d3dDepthStencilView.Reset();
  m_renderTarget.Reset();
  m_depthStencil.Reset();
  m_d3dContext->Flush();
  LWL*/

  // Determine the render target size in pixels.
  UINT backBufferWidth = std::max<UINT>(m_outputSize.right - m_outputSize.left, 1);
  UINT backBufferHeight = std::max<UINT>(m_outputSize.bottom - m_outputSize.top, 1);

  LOG(INFO) << "Creating window dependent resources with backbuffer size w=" << backBufferWidth
            << ", h=" << backBufferHeight;

  resizePipelineResources(_renderPipeline, backBufferWidth, backBufferHeight);
}

void D3D12_device_resources::resizePipelineResources(
    Render_pipeline& renderPipeline,
    UINT backBufferWidth,
    UINT backBufferHeight) {

  // Must destroy existing resources before re-creating the pipeline
  for (auto n = 0; n < Render_pipeline::frameCount; ++n) {
    renderPipeline.rtvBackBuffer[n].Reset();
  }

  if (renderPipeline.swapChain) {
    // If the swap chain already exists, resize it.
    HRESULT hr = renderPipeline.swapChain->ResizeBuffers(
        m_backBufferCount,
        backBufferWidth,
        backBufferHeight,
        m_backBufferFormat,
        (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#ifdef _DEBUG
      char buff[64] = {};
      sprintf_s(
          buff,
          "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
          (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr);
      OutputDebugStringA(buff);
#endif
      // If the device was removed for any reason, a new device and swap chain will need to be
      // created.
      HandleDeviceLost();

      // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will
      // reenter this method and correctly set up the new device.
      return;
    }

    ThrowIfFailed(hr);
  } else {
    // Create a descriptor for the swap chain.
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_backBufferCount;
    swapChainDesc.BufferDesc.Width = backBufferWidth;
    swapChainDesc.BufferDesc.Height = backBufferHeight;
    swapChainDesc.BufferDesc.Format = m_backBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.OutputWindow = m_window;

    // Create a SwapChain from a Win32 window.
    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(m_dxgiFactory->CreateSwapChain(
        renderPipeline.commandQueue.Get(), &swapChainDesc, &swapChain));

    ThrowIfFailed(swapChain.As(&renderPipeline.swapChain));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to
    // the ALT+ENTER shortcut
    ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
  }

  // Handle color space settings for HDR
  updateColorSpace(renderPipeline.swapChain);

  // Create heap for render target view descriptors
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = Render_pipeline::frameCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
      &rtvHeapDesc, IID_PPV_ARGS(&renderPipeline.rtcDescriptorHeap)));

  // Create a render target view of the swap chain back buffers.
  CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
      renderPipeline.rtcDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

  renderPipeline.frameIndex = renderPipeline.swapChain->GetCurrentBackBufferIndex();
  renderPipeline.rtcDescriptorSize =
      m_d3dDevice->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);

  for (auto n = 0; n < Render_pipeline::frameCount; ++n) {
    ThrowIfFailed(
        renderPipeline.swapChain->GetBuffer(n, IID_PPV_ARGS(&renderPipeline.rtvBackBuffer[n])));
    m_d3dDevice->CreateRenderTargetView(renderPipeline.rtvBackBuffer[n].Get(), nullptr, rtvHandle);
    rtvHandle.Offset(1, renderPipeline.rtcDescriptorSize);
  }

  /*LWL
  if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
    // Create a depth stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        m_depthBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL);
    // BEGIN LEWIS
    depthStencilDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    // END LEWIS

    ThrowIfFailed(m_d3dDevice->CreateTexture2D(
        &depthStencilDesc, nullptr, m_depthStencil.ReleaseAndGetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    // BEGIN LEWIS
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // END LEWIS

    ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(
        m_depthStencil.Get(),
        &depthStencilViewDesc,
        m_d3dDepthStencilView.ReleaseAndGetAddressOf()));
  }
      LWL*/

  // Set the 3D rendering viewport to target the entire window.
  m_screenViewport = CD3DX12_VIEWPORT(
      0.0f, 0.0f, static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight));
}

void D3D12_device_resources::createRenderPipeline(Render_pipeline& renderPipeline) {
  // create the command queue
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  ThrowIfFailed(
      m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&renderPipeline.commandQueue)));

  ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&renderPipeline.commandAllocator)));
}

bool D3D12_device_resources::checkSystemSupport(bool logFailures) {
  if (!XMVerifyCPUSupport()) {
    if (logFailures) {
      LOG(WARNING) << "Unsupported CPU";
    }
    return false;
  }

  return true;
}

bool D3D12_device_resources::getWindowSize(int& width, int& height) {
  width = m_outputSize.right;
  height = m_outputSize.bottom;
  return true;
}

// This method is called when the Win32 window changes size
bool D3D12_device_resources::setWindowSize(int width, int height) {
  RECT newRc;
  newRc.left = newRc.top = 0;
  newRc.right = width;
  newRc.bottom = height;
  if (newRc.bottom == m_outputSize.bottom && newRc.top == m_outputSize.top &&
      newRc.left == m_outputSize.left && newRc.right == m_outputSize.right) {
    // Handle color space settings for HDR
    updateColorSpace(_renderPipeline.swapChain);

    return false;
  }

  m_outputSize = newRc;

  if (m_d3dDevice) {
    recreateWindowSizeDependentResources();
  }

  return true;
}

// Recreate all device resources and set them back to the current state.
void D3D12_device_resources::HandleDeviceLost() {
  if (m_deviceNotify) {
    m_deviceNotify->onDeviceLost();
  }

  destroyDeviceDependentResources();

#ifdef _DEBUG
  {
    ComPtr<ID3D12DebugDevice1> d3dDebugDevice;
    if (SUCCEEDED(m_d3dDevice->QueryInterface<ID3D12DebugDevice1>(&d3dDebugDevice))) {
      d3dDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY);
    } else {
      OutputDebugStringA("Failed to get d3dDebugDevice");
    }
  }
#endif

  m_d3dDevice.Reset();
  m_dxgiFactory.Reset();

  createDeviceDependentResources();
  recreateWindowSizeDependentResources();

  if (m_deviceNotify) {
    m_deviceNotify->onDeviceRestored();
  }
}

// Present the contents of the swap chain to the screen.
void D3D12_device_resources::Present() {
  HRESULT hr;
  if (m_options & c_AllowTearing) {
    // Recommended to always use tearing if supported when using a sync interval of 0.
    hr = _renderPipeline.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
  } else {
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    hr = _renderPipeline.swapChain->Present(1, 0);
  }

  /*LWL
  // Discard the contents of the render target.
  // This is a valid operation only when the existing contents will be entirely
  // overwritten. If dirty or scroll rects are used, this call should be removed.
  m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

  if (m_d3dDepthStencilView) {
    // Discard the contents of the depth stencil.
    m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());
  }
  LWL*/

  // If the device was removed either by a disconnection or a driver upgrade, we
  // must recreate all device resources.
  if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#ifdef _DEBUG
    char buff[64] = {};
    sprintf_s(
        buff,
        "Device Lost on Present: Reason code 0x%08X\n",
        (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr);
    OutputDebugStringA(buff);
#endif
    HandleDeviceLost();
  } else {
    ThrowIfFailed(hr);

    if (!m_dxgiFactory->IsCurrent()) {
      // Output information is cached on the DXGI Factory. If it is stale we need to create a new
      // factory.
      CreateFactory();
    }
  }
}

void D3D12_device_resources::CreateFactory() {
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
  bool debugDXGI = false;
  {
    ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())))) {
      debugDXGI = true;

      ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_dxgiFactory)));

      dxgiInfoQueue->SetBreakOnSeverity(
          DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      dxgiInfoQueue->SetBreakOnSeverity(
          DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }
  }

  if (!debugDXGI)
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));
}

// This method acquires the first available hardware adapter.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
void D3D12_device_resources::GetHardwareAdapter(IDXGIAdapter1** ppAdapter) {
  *ppAdapter = nullptr;

  ComPtr<IDXGIAdapter1> adapter;
  for (UINT adapterIndex = 0;
       DXGI_ERROR_NOT_FOUND !=
       m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter);
       adapterIndex++) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      // Don't select the Basic Render Driver adapter.
      continue;
    }

#ifdef _DEBUG
    wchar_t buff[256] = {};
    swprintf_s(
        buff,
        L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n",
        adapterIndex,
        desc.VendorId,
        desc.DeviceId,
        desc.Description);
    OutputDebugStringW(buff);
#endif

    break;
  }

  *ppAdapter = adapter.Detach();
}

// Sets the color space for the swap chain in order to handle HDR output.
void D3D12_device_resources::updateColorSpace(const ComPtr<IDXGISwapChain>& swapChain) {
  DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

  bool isDisplayHDR10 = false;

#if defined(NTDDI_WIN10_RS2)
  if (swapChain) {
    ComPtr<IDXGIOutput> output;
    if (SUCCEEDED(swapChain->GetContainingOutput(output.GetAddressOf()))) {
      ComPtr<IDXGIOutput6> output6;
      if (SUCCEEDED(output.As(&output6))) {
        DXGI_OUTPUT_DESC1 desc;
        ThrowIfFailed(output6->GetDesc1(&desc));

        if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
          // Display output is HDR10.
          isDisplayHDR10 = true;
        }
      }
    }
  }
#endif

  if ((m_options & c_EnableHDR) && isDisplayHDR10) {
    switch (m_backBufferFormat) {
    case DXGI_FORMAT_R10G10B10A2_UNORM:
      // The application creates the HDR10 signal.
      colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
      break;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
      // The system creates the HDR10 signal; application uses linear values.
      colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
      break;

    default:
      break;
    }
  }

  m_colorSpace = colorSpace;

  ComPtr<IDXGISwapChain3> swapChain3;
  if (swapChain && SUCCEEDED(swapChain.As(&swapChain3))) {
    UINT colorSpaceSupport = 0;
    if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)) &&
        (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
      ThrowIfFailed(swapChain3->SetColorSpace1(colorSpace));
    }
  }
}
