//
// DeviceResources.h - A wrapper for the Direct3D 12 device and swapchain
//

#pragma once

#include "OeCore/IDevice_resources.h"

#include "D3D12_vendor.h"
#include "D3D12_renderer_types.h"
#include "Descriptor_heap_pool.h"

#include <ResourceUploadBatch.h>
#include <wrl/client.h>

namespace oe::pipeline_d3d12 {

class Frame_resources;

// Controls all the DirectX device resources.
class D3D12_device_resources : public IDevice_resources {
 public:
  static const unsigned int c_AllowTearing = 0x1;
  static const unsigned int c_EnableHDR = 0x2;

  D3D12_device_resources(
          HWND window, DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
          DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT, UINT backBufferCount = 2, unsigned int flags = 0);

  void createDeviceDependentResources() override;
  void recreateWindowSizeDependentResources() override;

  void beginResourcesUploadStep() override;
  void endResourcesUploadStep() override;

  void destroyDeviceDependentResources() override;
  bool getWindowSize(int& width, int& height) override;
  bool setWindowSize(int width, int height) override;
  bool checkSystemSupport(bool logFailures) override;

  // Primary pipeline execution flow
  void waitForFenceAndReset();
  void present();

  // Device Accessors.
  RECT GetOutputSize() const
  {
    return m_outputSize;
  }

  // Direct3D Accessors.
  ID3D12Device6* GetD3DDevice() const
  {
    return m_d3dDevice.Get();
  }
  D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const
  {
    return m_d3dFeatureLevel;
  }
  // ID3D11Texture2D*        GetRenderTarget() const                 { return m_renderTarget.Get();
  // } ID3D11Texture2D*        GetDepthStencil() const                 { return
  // m_depthStencil.Get(); } ID3D11RenderTargetView*	GetRenderTargetView() const             {
  // return m_d3dRenderTargetView.Get(); } ID3D11DepthStencilView* GetDepthStencilView() const {
  // return m_d3dDepthStencilView.Get(); }
  DXGI_FORMAT GetBackBufferFormat() const
  {
    return m_backBufferFormat;
  }
  DXGI_FORMAT GetDepthBufferFormat() const
  {
    return m_depthBufferFormat;
  }
  const D3D12_VIEWPORT& GetScreenViewport() const
  {
    return m_screenViewport;
  }
  UINT GetBackBufferCount() const
  {
    return m_backBufferCount;
  }
  DXGI_COLOR_SPACE_TYPE GetColorSpace() const
  {
    return m_colorSpace;
  }
  unsigned int GetDeviceOptions() const
  {
    return m_options;
  }

  ID3D12CommandQueue* GetCommandQueue() const;

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> GetPipelineCommandList();
  DirectX::ResourceUploadBatch& getResourceUploader() { return *_renderPipeline.resourceUploadBatch; }

  void PIXBeginEvent(const wchar_t* label);
  void PIXBeginEvent(const char* label) {
    PIXBeginEvent(oe::utf8_decode(label).c_str());
  }

  void PIXEndEvent(void);

  void PIXSetMarker(const wchar_t* label);
  void PIXSetMarker(const char* label) {
    PIXBeginEvent(oe::utf8_decode(label).c_str());
  }

  Committed_gpu_resource getCurrentFrameBackBuffer() const;
  Vector2u getBackBufferDimensions() const;
  Committed_gpu_resource getDepthStencil() const;
  DXGI_FORMAT getDepthStencilFormat();

  Frame_resources& getCurrentFrameResources();
  size_t getCurrentFrameIndex() const;

  Descriptor_heap_pool& getRtvHeap() { return *m_rtvDescriptorHeapPool; }
  Descriptor_heap_pool& getDsvHeap() { return *m_dsvDescriptorHeapPool; }
  Descriptor_heap_pool& getSrvHeap() { return *m_srvDescriptorHeapPool; }
  Descriptor_heap_pool& getSamplerHeap() { return *m_samplerDescriptorHeapPool; }

  void addReferenceToInFlightFrames(const Microsoft::WRL::ComPtr<IUnknown>& ptr);

 private:
  struct Render_pipeline {
    static const std::size_t frameCount = 2;
    static const std::size_t frameResourceCount = 2;

    // Pipeline Objects
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;

    Descriptor_range rtvDescriptorRange;
    Descriptor_range dsvDescriptorRange;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, frameCount> backBufferRtvs;
    UINT frameIndex;

    std::array<std::unique_ptr<Frame_resources>, frameResourceCount> frameResources;
    size_t currentFrameResources = 0;

    HANDLE fenceEvent;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;

    std::unique_ptr<DirectX::ResourceUploadBatch> resourceUploadBatch;

    // TODO: Below here needs to move to Frame_resources
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    // TODO: Below here needs to move various pipelines
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList;
  };

  void HandleDeviceLost();
  void CreateFactory();
  void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);

  void createRenderPipeline(Render_pipeline& renderPipeline);

  void resizePipelineResources(Render_pipeline& renderPipeline, UINT backBufferWidth, UINT backBufferHeight);
  void waitForPreviousFrame(Render_pipeline& renderPipeline);

  // If a null colour space is passed, just sets the state of m_colorSpace
  void updateColorSpace(const Microsoft::WRL::ComPtr<IDXGISwapChain>& swapChain);


  // Direct3D objects.
  Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
  Microsoft::WRL::ComPtr<ID3D12Device6> m_d3dDevice;

  D3D12_VIEWPORT m_screenViewport;

  inline static constexpr uint32_t kDescriptorPoolSizeDsvRtv = 128;
  inline static constexpr uint32_t kDescriptorPoolSizeSrv = 10240;
  inline static constexpr uint32_t kDescriptorPoolSizeSampler = 1024;
  std::unique_ptr<Descriptor_heap_pool> m_rtvDescriptorHeapPool;
  std::unique_ptr<Descriptor_heap_pool> m_dsvDescriptorHeapPool;
  std::unique_ptr<Descriptor_heap_pool> m_srvDescriptorHeapPool;
  std::unique_ptr<Descriptor_heap_pool> m_samplerDescriptorHeapPool;

  // Direct3D properties.
  DXGI_FORMAT m_backBufferFormat;
  DXGI_FORMAT m_depthBufferFormat;
  UINT m_backBufferCount;

  // Cached device properties.
  HWND m_window;
  inline static constexpr D3D_FEATURE_LEVEL m_d3dFeatureLevel = D3D_FEATURE_LEVEL_12_1;
  RECT m_outputSize;

  // HDR Support
  DXGI_COLOR_SPACE_TYPE m_colorSpace;

  Render_pipeline _renderPipeline;

  // DeviceResources options (see flags above)
  unsigned int m_options;

  // If true, outputs detailed DirectX memory information on shutdown
  bool m_outputDetailedMemoryReport;
  void closeAndExecuteCommandList(Render_pipeline& renderPipeline);
};
}// namespace oe
