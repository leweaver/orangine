//
// DeviceResources.h - A wrapper for the Direct3D 12 device and swapchain
//

#pragma once

#include "OeCore/IDevice_resources.h"

#include "Descriptor_heap_pool.h"

#include "D3D12_vendor.h"
#include <wrl/client.h>

namespace oe {

// Controls all the DirectX device resources.
class D3D12_device_resources : public IDevice_resources {
 public:
  static const unsigned int c_AllowTearing = 0x1;
  static const unsigned int c_EnableHDR = 0x2;

  D3D12_device_resources(
      HWND window,
      DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
      DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
      UINT backBufferCount = 2,
      unsigned int flags = 0);

  // IDevice_resources interface
  void registerDeviceNotify(IDevice_notify* deviceNotify) override {
    m_deviceNotify = deviceNotify;
  }

  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;
  bool getWindowSize(int& width, int& height) override;
  bool setWindowSize(int width, int height) override;
  void recreateWindowSizeDependentResources() override;
  bool checkSystemSupport(bool logFailures) override;



  void Present();

  // Device Accessors.
  RECT GetOutputSize() const { return m_outputSize; }

  // Direct3D Accessors.
  ID3D12Device6* GetD3DDevice() const { return m_d3dDevice.Get(); }
  D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
  // ID3D11Texture2D*        GetRenderTarget() const                 { return m_renderTarget.Get();
  // } ID3D11Texture2D*        GetDepthStencil() const                 { return
  // m_depthStencil.Get(); } ID3D11RenderTargetView*	GetRenderTargetView() const             {
  // return m_d3dRenderTargetView.Get(); } ID3D11DepthStencilView* GetDepthStencilView() const {
  // return m_d3dDepthStencilView.Get(); }
  DXGI_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
  DXGI_FORMAT GetDepthBufferFormat() const { return m_depthBufferFormat; }
  const D3D12_VIEWPORT& GetScreenViewport() const { return m_screenViewport; }
  UINT GetBackBufferCount() const { return m_backBufferCount; }
  DXGI_COLOR_SPACE_TYPE GetColorSpace() const { return m_colorSpace; }
  unsigned int GetDeviceOptions() const { return m_options; }

  ID3D12CommandQueue* GetCommandQueue() const;

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GetPipelineCommandList();

  // Performance events
  void PIXBeginEvent(_In_z_ const wchar_t* name) {
    // m_d3dAnnotation->BeginEvent(name);
  }

  void PIXEndEvent() {
    // m_d3dAnnotation->EndEvent();
  }

  void PIXSetMarker(_In_z_ const wchar_t* name) {
    // m_d3dAnnotation->SetMarker(name);
  }

  struct Committed_gpu_resource {
    ID3D12Resource* resource;
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  };
  Committed_gpu_resource getCurrentFrameBackBuffer() const;
  Committed_gpu_resource getDepthStencil() const;

 private:

  struct Command_list_state  {
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;

    // Synchronization objects.
    UINT frameIndex;
    HANDLE fenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue;
  };

  // TODO: Some parts of this probably belong in Render_step_manager (command queue/list/allocator)
  struct Render_pipeline {
    static const std::size_t frameCount = 2;

    // Pipeline Objects
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
    pipeline_d3d12::Descriptor_heap_pool::Descriptor_range rtvDescriptorRange;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, frameCount> rtvBackBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;

    // TODO: will want more than one command list :)
    Command_list_state commandList;
  };

  void HandleDeviceLost();
  void CreateFactory();
  void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);

  void createRenderPipeline(Render_pipeline& renderPipeline);

  void resizePipelineResources(
      Render_pipeline& renderPipeline,
      UINT backBufferWidth,
      UINT backBufferHeight);
  void waitForPreviousFrame(Render_pipeline& renderPipeline);

  // If a null colour space is passed, just sets the state of m_colorSpace
  void updateColorSpace(const Microsoft::WRL::ComPtr<IDXGISwapChain>& swapChain);



  // Direct3D objects.
  Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
  Microsoft::WRL::ComPtr<ID3D12Device6> m_d3dDevice;
  // Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>   m_d3dAnnotation;


  // Direct3D rendering objects. Required for 3D.
  // Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_renderTarget;
  // Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthStencil;
  // Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_d3dRenderTargetView;
  // Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_d3dDepthStencilView;
  D3D12_VIEWPORT m_screenViewport;
  std::unique_ptr<pipeline_d3d12::Descriptor_heap_pool> m_rtvDescriptorHeapPool;
  std::unique_ptr<pipeline_d3d12::Descriptor_heap_pool> m_dsvDescriptorHeapPool;

  // Direct3D properties.
  DXGI_FORMAT m_backBufferFormat;
  DXGI_FORMAT m_depthBufferFormat;
  UINT m_backBufferCount;

  // Cached device properties.
  HWND m_window;
  static constexpr D3D_FEATURE_LEVEL m_d3dFeatureLevel = D3D_FEATURE_LEVEL_12_1;
  RECT m_outputSize;

  // HDR Support
  DXGI_COLOR_SPACE_TYPE m_colorSpace;

  Render_pipeline _renderPipeline;

  // DeviceResources options (see flags above)
  unsigned int m_options;

  // The IDeviceNotify can be held directly as it owns the DeviceResources.
  IDevice_notify* m_deviceNotify;

  // If true, outputs detailed DirectX memory information on shutdown
  bool m_outputDetailedMemoryReport;
  Command_list_state createCommandListState();
};
} // namespace oe
