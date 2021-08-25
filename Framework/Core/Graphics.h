#pragma once
#define FrameCount 3

class Graphics
{
public:
	Graphics();
	~Graphics();
public:
	void LoadPipeline();
	void LoadAssets();
	void LoadSizeDependentResources();
	void EnumerateGPUadapters();
    void ReleaseSizeDependentResources();
	void RecreateD3Dresources();
	void ReleaseD3DObjects();
	void GetGPUAdapter(UINT adapterIndex, IDXGIAdapter1** ppAdapter);
	bool QueryForAdapterEnumerationChanges();
	HRESULT ValidateActiveAdapter();
	bool RetrieveAdapterIndex(UINT* adapterIndex, LUID prevActiveAdapterLuid);
	void SelectAdapter(UINT index);
	void SelectGPUPreference(UINT index);
	void CalculateFrameStats();
	void WaitForGpu(ID3D12CommandQueue* pCommandQueue);
	void MoveToNextFrame();
public:
	void Initiallize();
	void Update();
	void Render();

private:
	// GPU adapter management.
	struct DxgiAdapterInfo
	{
		DXGI_ADAPTER_DESC1 desc;
		bool supportsDx12FL11;
	};
	DXGI_GPU_PREFERENCE m_activeGpuPreference;
	std::map<DXGI_GPU_PREFERENCE, std::wstring> m_gpuPreferenceToName;
	UINT m_activeAdapter;
	LUID m_activeAdapterLuid;
	std::vector<DxgiAdapterInfo> m_gpuAdapterDescs;
	bool m_manualAdapterSelection;
	HANDLE m_adapterChangeEvent;
	DWORD m_adapterChangeRegistrationCookie;

	// D3D objects.
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
#ifdef USE_DXGI_1_6
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<IDXGIFactory6>   m_dxgiFactory;
#else
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<IDXGIFactory2>   m_dxgiFactory;
#endif
	UINT m_dxgiFactoryFlags;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];


	ComPtr<ID3D12Fence> m_fence;
	UINT   m_frameIndex;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValues[FrameCount];

	// Scene rendering resources
	std::unique_ptr<class Scene> m_scene;
	
private:
	// Pipeline objects.

	uint m_width;
	uint m_height;
};