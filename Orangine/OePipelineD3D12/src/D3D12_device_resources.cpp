//
// D3D12_device_resources.cpp - A wrapper for the Direct3D 11 device and swapchain
//                       (requires DirectX 11.1 Runtime)
//

#include "D3D12_device_resources.h"
#include "OeCore/EngineUtils.h"

#include <wrl/client.h>
#include "PipelineUtils.h"
#include "Frame_resources.h"

#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameters in PIXCopyEventArguments() (WinPixEventRuntime.1.0.200127001)
#include <pix3.h>
#pragma warning(pop)

using namespace DirectX;
using namespace oe::pipeline_d3d12;

using Microsoft::WRL::ComPtr;

// Constructor for D3D12_device_resources.
D3D12_device_resources::D3D12_device_resources(
    HWND hwnd,
    DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    UINT backBufferCount,
    unsigned int flags)
    : m_screenViewport{}
    , m_backBufferFormat(backBufferFormat)
    , m_depthBufferFormat(depthBufferFormat)
    , m_backBufferCount(backBufferCount)
    , m_window(hwnd)
    , m_outputSize{0, 0, 1, 1}
    , m_colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
    , m_options(flags)
    , m_outputDetailedMemoryReport(true) {}

// Configures the Direct3D device, and stores handles to it and the device context.
void D3D12_device_resources::createDeviceDependentResources()
{
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
      LOG(WARNING) << "Variable refresh rate displays not supported";
#endif
    }
  }

  // Disable HDR if we are on an OS that can't support FLIP swap effects
  if (m_options & c_EnableHDR) {
    ComPtr<IDXGIFactory5> factory5;
    if (FAILED(m_dxgiFactory.As(&factory5))) {
      m_options &= ~c_EnableHDR;
#ifdef _DEBUG
      LOG(WARNING) << "Flip swap effects not supported";
#endif
    }
  }

  ComPtr<IDXGIAdapter1> adapter;
  GetHardwareAdapter(adapter.GetAddressOf());

  // Create the Direct3D 11 API device object and a corresponding context.
  ComPtr<ID3D12Device> device;
  HRESULT hr = E_FAIL;
  if (adapter) {
    hr = D3D12CreateDevice(adapter.Get(), m_d3dFeatureLevel, IID_PPV_ARGS(&device));
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

    hr = D3D12CreateDevice(warpAdapter.Get(), m_d3dFeatureLevel, IID_PPV_ARGS(&device));

    if (SUCCEEDED(hr)) {
      LOG(INFO) << "Created WARP Direct3D Adapter";
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
  m_rtvDescriptorHeapPool = {};
  m_dsvDescriptorHeapPool = {};
  m_srvDescriptorHeapPool = {};
  m_samplerDescriptorHeapPool = {};

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

  // Determine the render target size in pixels.
  UINT backBufferWidth = std::max<UINT>(m_outputSize.right - m_outputSize.left, 1);
  UINT backBufferHeight = std::max<UINT>(m_outputSize.bottom - m_outputSize.top, 1);

  LOG(INFO) << "Creating window dependent resources with backbuffer size w=" << backBufferWidth
            << ", h=" << backBufferHeight;

  resizePipelineResources(_renderPipeline, backBufferWidth, backBufferHeight);

  ++_renderPipeline.fenceValue;
  ThrowIfFailed(_renderPipeline.commandQueue->Signal(_renderPipeline.fence.Get(), _renderPipeline.fenceValue));
}


void D3D12_device_resources::beginResourcesUploadStep()
{
  _renderPipeline.resourceUploadBatch->Begin();
}

void D3D12_device_resources::endResourcesUploadStep()
{
  auto future = _renderPipeline.resourceUploadBatch->End(GetCommandQueue());
  future.wait();
}

void D3D12_device_resources::resizePipelineResources(
    Render_pipeline& renderPipeline,
    UINT backBufferWidth,
    UINT backBufferHeight) {

  // Must destroy existing resources before re-creating the pipeline
  for (auto n = 0; n < Render_pipeline::frameCount; ++n) {
    renderPipeline.backBufferRtvs.at(n).Reset();
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
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = m_backBufferCount;
    swapChainDesc.Width = backBufferWidth;
    swapChainDesc.Height = backBufferHeight;
    swapChainDesc.Format = m_backBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain1> swapChain;
    // Create a SwapChain from a Win32 window.
    ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
            renderPipeline.commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
            m_window,
            &swapChainDesc,
            &swapChainFullscreenDesc,
            nullptr,
            &swapChain
            ));

    // This class does not support exclusive full-screen mode and prevents DXGI from responding to
    // the ALT+ENTER shortcut
    ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&renderPipeline.swapChain));
  }

  // Handle color space settings for HDR
  updateColorSpace(renderPipeline.swapChain);

  // Initialize descriptor heaps
  m_rtvDescriptorHeapPool->initialize();
  m_dsvDescriptorHeapPool->initialize();
  m_srvDescriptorHeapPool->initialize();
  m_samplerDescriptorHeapPool->initialize();

  // Create render target views for back buffer
  {
    // Create a render target view of the swap chain back buffers.
    renderPipeline.rtvDescriptorRange = m_rtvDescriptorHeapPool->allocateRange(Render_pipeline::frameCount);

    renderPipeline.frameIndex = renderPipeline.swapChain->GetCurrentBackBufferIndex();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeapPool->getCpuDescriptorHandleForHeapStart());
    for (auto n = 0; n < Render_pipeline::frameCount; ++n) {
      auto& backBufferRtv = _renderPipeline.backBufferRtvs.at(n);
      ThrowIfFailed(renderPipeline.swapChain->GetBuffer(n, IID_PPV_ARGS(&backBufferRtv)));
      m_d3dDevice->CreateRenderTargetView(backBufferRtv.Get(), nullptr, rtvHandle);
      rtvHandle.Offset(1, renderPipeline.rtvDescriptorRange.incrementSize);

      oe::pipeline_d3d12::SetObjectNameIndexed(backBufferRtv.Get(), L"rtvBackBuffer", n);
    }
  }

  // Create the depth stencil view.
  {
    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = getDepthStencilFormat();
    depthOptimizedClearValue.DepthStencil.Depth = 1.0F;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    auto resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            getDepthStencilFormat(), backBufferWidth, backBufferHeight, 1, 1, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    // TODO: What is d3d12 equiv of this? Do I need to toggle state between D3D12_RESOURCE_STATE_DEPTH_READ and
    //       D3D12_RESOURCE_STATE_DEPTH_WRITE between render steps?
    //       Alternative - do a copy from depth -> another texture that's accessible from shaders. What's the perf difference?
    // depthStencilDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, resourceState, &depthOptimizedClearValue,
            IID_PPV_ARGS(&renderPipeline.depthStencilBuffer)));

    oe::pipeline_d3d12::SetObjectName(renderPipeline.depthStencilBuffer.Get(), L"depthStencilBuffer");

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = getDepthStencilFormat();
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    _renderPipeline.dsvDescriptorRange = m_dsvDescriptorHeapPool->allocateRange(1);
    m_d3dDevice->CreateDepthStencilView(
            renderPipeline.depthStencilBuffer.Get(), &depthStencilDesc,
            _renderPipeline.dsvDescriptorRange.cpuHandle);
  }

  // Set the 3D rendering viewport to target the entire window.
  m_screenViewport = CD3DX12_VIEWPORT(
      0.0F, 0.0F, static_cast<float>(backBufferWidth), static_cast<float>(backBufferHeight));

  // Close the command list and execute it to begin the initial GPU setup.
  closeAndExecuteCommandList(renderPipeline);

  // Create synchronization objects and wait until assets have been uploaded to the GPU.
  {
    _renderPipeline.fenceValue = 0;
    ThrowIfFailed(m_d3dDevice->CreateFence(_renderPipeline.fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_renderPipeline.fence)));
    _renderPipeline.fenceValue++;

    // Create an event handle to use for frame synchronization.
    _renderPipeline.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (_renderPipeline.fenceEvent == nullptr)
    {
      ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
  }
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


  auto type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ThrowIfFailed(m_d3dDevice->CreateCommandList(0, type, _renderPipeline.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&renderPipeline.commandList)));
  SetObjectName(renderPipeline.commandList.Get(), L"RenderCommandList");

  renderPipeline.resourceUploadBatch = std::make_unique<DirectX::ResourceUploadBatch>(m_d3dDevice.Get());

  for (size_t idx = 0; idx < Render_pipeline::frameResourceCount; ++idx)
  {
    renderPipeline.frameResources.at(idx) = std::make_unique<Frame_resources>(m_d3dDevice.Get());
  }

  // Create Descriptor heap pools
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
          D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kDescriptorPoolSizeDsvRtv, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
  m_rtvDescriptorHeapPool = std::make_unique<pipeline_d3d12::Descriptor_heap_pool>(
          m_d3dDevice.Get(), rtvHeapDesc, Render_pipeline::frameCount, "rtvDescriptorHeapPool");

  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {
          D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kDescriptorPoolSizeDsvRtv, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
  m_dsvDescriptorHeapPool = std::make_unique<pipeline_d3d12::Descriptor_heap_pool>(
          m_d3dDevice.Get(), dsvHeapDesc, Render_pipeline::frameCount, "dsvDescriptorHeapPool");

  D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kDescriptorPoolSizeSrv, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
  m_srvDescriptorHeapPool = std::make_unique<Descriptor_heap_pool>(
          m_d3dDevice.Get(), srvHeapDesc, Render_pipeline::frameCount, "srvDescriptorHeapPool");

  D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{
          D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kDescriptorPoolSizeSampler, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
  m_samplerDescriptorHeapPool = std::make_unique<Descriptor_heap_pool>(
          m_d3dDevice.Get(), samplerHeapDesc, Render_pipeline::frameCount, "samplerDescriptorHeapPool");
}

Committed_gpu_resource D3D12_device_resources::getCurrentFrameBackBuffer() const
{
  int frameIdx = static_cast<int>(_renderPipeline.frameIndex);
  Committed_gpu_resource data{
          _renderPipeline.backBufferRtvs.at(frameIdx).Get(),
          _renderPipeline.rtvDescriptorRange.cpuHandle, _renderPipeline.rtvDescriptorRange.gpuHandle};

  data.cpuHandle.Offset(frameIdx, _renderPipeline.rtvDescriptorRange.incrementSize);
  data.gpuHandle.Offset(frameIdx, _renderPipeline.rtvDescriptorRange.incrementSize);

  return data;
}

oe::Vector2u D3D12_device_resources::getBackBufferDimensions() const
{
  return {static_cast<uint32_t>(m_outputSize.right - m_outputSize.left),
          static_cast<uint32_t>(m_outputSize.bottom - m_outputSize.top)};
}

Committed_gpu_resource D3D12_device_resources::getDepthStencil() const
{
  return Committed_gpu_resource{
          _renderPipeline.depthStencilBuffer.Get(),
          CD3DX12_CPU_DESCRIPTOR_HANDLE(_renderPipeline.dsvDescriptorRange.cpuHandle),
          CD3DX12_GPU_DESCRIPTOR_HANDLE(_renderPipeline.dsvDescriptorRange.gpuHandle)};
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
}

void D3D12_device_resources::closeAndExecuteCommandList(D3D12_device_resources::Render_pipeline& renderPipeline)
{
  ID3D12GraphicsCommandList4* commandList = _renderPipeline.commandList.Get();

  // Close the command list
  ThrowIfFailed(commandList->Close());

  // Execute
  std::array<ID3D12CommandList*, 1> ppCommandLists = {commandList};
  renderPipeline.commandQueue->ExecuteCommandLists(ppCommandLists.size(), ppCommandLists.data());
}

void D3D12_device_resources::waitForFenceAndReset()
{
  waitForPreviousFrame(_renderPipeline);

  // Command list allocators can only be reset when the associated
  // command lists have finished execution on the GPU; apps should use
  // fences to determine GPU execution progress.
  ThrowIfFailed(_renderPipeline.commandAllocator->Reset());

  // However, when ExecuteCommandList() is called on a particular command
  // list, that command list can then be reset at any time and must be before
  // re-recording.
  ThrowIfFailed(_renderPipeline.commandList->Reset(_renderPipeline.commandAllocator.Get(), nullptr));

  // Indicate that the back buffer will be used as a render target.
  ID3D12Resource* backBuffer = _renderPipeline.backBufferRtvs.at(_renderPipeline.frameIndex).Get();
  CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
          backBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
  _renderPipeline.commandList->ResourceBarrier(1, &resourceBarrier);

  // Flush heaps
  m_rtvDescriptorHeapPool->handleFrameStart(_renderPipeline.frameIndex);
  m_dsvDescriptorHeapPool->handleFrameStart(_renderPipeline.frameIndex);
  m_srvDescriptorHeapPool->handleFrameStart(_renderPipeline.frameIndex);
  m_samplerDescriptorHeapPool->handleFrameStart(_renderPipeline.frameIndex);
}

void D3D12_device_resources::waitForPreviousFrame(Render_pipeline& renderPipeline)
{
  const UINT64 lastCompletedFence = renderPipeline.fence->GetCompletedValue();

  // Make sure that this frame resource isn't still in use by the GPU.
  // If it is, wait for it to complete.
  if (renderPipeline.fenceValue != 0 && renderPipeline.fenceValue > lastCompletedFence)
  {
    ThrowIfFailed(renderPipeline.fence->SetEventOnCompletion(renderPipeline.fenceValue, renderPipeline.fenceEvent));
    WaitForSingleObject(renderPipeline.fenceEvent, INFINITE);
  }
}

// Present the contents of the swap chain to the screen.
void D3D12_device_resources::present() {
  if (!_renderPipeline.swapChain) {
    return;
  }

  // Transition backbuffer to present
  ID3D12Resource* backBuffer = _renderPipeline.backBufferRtvs.at(_renderPipeline.frameIndex).Get();
  CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
          backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

  // Indicate that the back buffer will now be used to present.
  _renderPipeline.commandList->ResourceBarrier(1, &resourceBarrier);

  closeAndExecuteCommandList(_renderPipeline);

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

  // handle device lost and DXGI factory recreation
  {
    // If the device was removed either by a disconnection or a driver upgrade, we
    // must recreate all device resources.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
      if constexpr (_DEBUG) {
        char buff[64] = {};
        sprintf_s(
                buff, "Device Lost on Present: Reason code 0x%08X\n",
                (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr);
        OutputDebugStringA(buff);
      }
      HandleDeviceLost();
    }
    else {
      ThrowIfFailed(hr);

      if (!m_dxgiFactory->IsCurrent()) {
        // Output information is cached on the DXGI Factory. If it is stale we need to create a new
        // factory.
        CreateFactory();
      }
    }
  }

  // Update backbuffer index after present, which progresses the swapchain
  _renderPipeline.frameIndex = _renderPipeline.swapChain->GetCurrentBackBufferIndex();

  // increment the fence value and signal.
  ++_renderPipeline.fenceValue;
  ThrowIfFailed(_renderPipeline.commandQueue->Signal(_renderPipeline.fence.Get(), _renderPipeline.fenceValue));
}

void D3D12_device_resources::CreateFactory()
{
  UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
  ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
  }
  else {
    LOG(WARNING) << "WARNING: Direct3D Debug Info Queue is not available";
  }

  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    debugController->EnableDebugLayer();
  }
  else {
    LOG(WARNING) << "WARNING: Direct3D Debug Controller is not available";
  }
#endif

  ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));
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

ID3D12CommandQueue* D3D12_device_resources::GetCommandQueue() const
{
  return _renderPipeline.commandQueue.Get();
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> D3D12_device_resources::GetPipelineCommandList()
{
  return _renderPipeline.commandList;
}

DXGI_FORMAT D3D12_device_resources::getDepthStencilFormat() {
  return DXGI_FORMAT_D24_UNORM_S8_UINT;
}

void D3D12_device_resources::PIXBeginEvent(const wchar_t* label)
{
#ifdef RELEASE
  (label);
#else
  ::PIXBeginEvent(_renderPipeline.commandList.Get(), 0, label);
#endif
}

void D3D12_device_resources::PIXEndEvent(void)
{
#ifndef RELEASE
  ::PIXEndEvent(_renderPipeline.commandList.Get());
#endif
}

void D3D12_device_resources::PIXSetMarker(const wchar_t* label)
{
#ifdef RELEASE
  (label);
#else
  ::PIXSetMarker(_renderPipeline.commandList.Get(), 0, label);
#endif
}

Frame_resources& D3D12_device_resources::getCurrentFrameResources()
{
  return *_renderPipeline.frameResources.at(_renderPipeline.currentFrameResources);
}

size_t D3D12_device_resources::getCurrentFrameIndex() const
{
  return _renderPipeline.frameIndex;
}

void D3D12_device_resources::addReferenceToInFlightFrames(const ComPtr<IUnknown>& ptr) {
  for (auto& frameReference : _renderPipeline.frameResources) {
    frameReference->addReferenceForFrame(ptr);
  }
}
