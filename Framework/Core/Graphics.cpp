#include "Framework.h"
#include "Graphics.h"
#include <dxgidebug.h>
#include "Scene/Scene.h"
Graphics::Graphics()
	: 
	m_frameIndex(0),
	m_activeAdapter(0),
	m_activeGpuPreference(DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE),
	m_manualAdapterSelection(false),
	m_adapterChangeEvent(NULL),
	m_adapterChangeRegistrationCookie(0),
	m_fenceValues{},
	m_dxgiFactoryFlags(0)
{
	m_width = D3D::Width();
	m_height = D3D::Height();


}

Graphics::~Graphics()
{
}

void Graphics::LoadPipeline()
{
	m_dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			m_dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	Check(CreateDXGIFactory2(m_dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));

#ifdef USE_DXGI_1_6
	ComPtr<IDXGIFactory7> spDxgiFactory7;
	if (SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&spDxgiFactory7))))
	{
		m_adapterChangeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_adapterChangeEvent == nullptr)
		{
			Check(HRESULT_FROM_WIN32(GetLastError()));
		}
		Check(spDxgiFactory7->RegisterAdaptersChangedEvent(m_adapterChangeEvent, &m_adapterChangeRegistrationCookie));
	}
#endif

	EnumerateGPUadapters();

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetGPUAdapter(m_activeAdapter, &hardwareAdapter);
	Check(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	));
	m_activeAdapterLuid = m_gpuAdapterDescs[m_activeAdapter].desc.AdapterLuid;

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	Check(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
	

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	// It is recommended to always use the tearing flag when it is available.
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> swapChain;
	// DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost).
	// Temporarily remove the topmost property for creating the swapchain.
	bool prevIsFullscreen = D3D::GetDesc().bFullScreen;
	if (prevIsFullscreen)
	{
		//Win32Application::SetWindowZorderToTopMost(false);
	}
	Check(m_dxgiFactory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		D3D::GetDesc().Handle,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	/*if (prevIsFullscreen)
	{
		Win32Application::SetWindowZorderToTopMost(true);
	}
*/
// With tearing support enabled we will handle ALT+Enter key presses in the
// window message loop rather than let DXGI handle it by calling SetFullscreenState.
	m_dxgiFactory->MakeWindowAssociation(D3D::GetDesc().Handle, DXGI_MWA_NO_ALT_ENTER);

	Check(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create synchronization objects.
	{
		Check(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValues[m_frameIndex]++;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			Check(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
}

void Graphics::LoadAssets()
{
	if (!m_scene)
	{
		m_scene = make_unique<Scene>(FrameCount);
	}

	// Create a temporary command queue and command list for initializing data on the GPU.
	// Performance tip: Copy command queues are optimized for transfer over PCIe.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

	ComPtr<ID3D12CommandQueue> copyCommandQueue;
	Check(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&copyCommandQueue)));
	
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	Check(m_device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(&commandAllocator)));
	

	ComPtr<ID3D12GraphicsCommandList> commandList;
	Check(m_device->CreateCommandList(0, queueDesc.Type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	
	m_scene->Initiallize(m_device.Get(), m_commandQueue.Get(), commandList.Get(), m_frameIndex);

	Check(commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	copyCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Wait until assets have been uploaded to the GPU.
	WaitForGpu(copyCommandQueue.Get());
}

void Graphics::LoadSizeDependentResources()
{
	for (UINT i = 0; i < FrameCount; i++)
	{
		Check(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
	}

	m_scene->LoadSizeDependentResources(m_device.Get(), m_renderTargets, m_width, m_height);
}

void Graphics::EnumerateGPUadapters()
{
	m_gpuAdapterDescs.clear();

	ComPtr<IDXGIAdapter1> adapter;
#ifdef USE_DXGI_1_6
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapterByGpuPreference(adapterIndex, m_activeGpuPreference, IID_PPV_ARGS(&adapter)); ++adapterIndex)
#else
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
#endif
	{
		DxgiAdapterInfo adapterInfo;
		Check(adapter->GetDesc1(&adapterInfo.desc));
		adapterInfo.supportsDx12FL11 = SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr));
		m_gpuAdapterDescs.push_back(move(adapterInfo));
	}
}

void Graphics::ReleaseSizeDependentResources()
{
	m_scene->ReleaseSizeDependentResources();
	//if (m_enableUI)
	//{
	//	m_uiLayer.reset();
	//}
	for (UINT i = 0; i < FrameCount; i++)
	{
		m_renderTargets[i].Reset();
	}
}

void Graphics::RecreateD3Dresources()
{
	// Give GPU a chance to finish its execution in progress.
	try
	{
		WaitForGpu(m_commandQueue.Get());
	}
	catch (HrException&)
	{
		// Do nothing, currently attached adapter is unresponsive.
	}
	ReleaseD3DObjects();
	Initiallize();
}

void Graphics::ReleaseD3DObjects()
{
	m_scene->ReleaseD3DObjects();

	m_fence.Reset();

	ResetComPtrArray(&m_renderTargets);
	m_commandQueue.Reset();
	m_swapChain.Reset();
	m_device.Reset();

#ifdef USE_DXGI_1_6
	ComPtr<IDXGIFactory7> spDxgiFactory7;
	if (m_adapterChangeRegistrationCookie != 0 && SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&spDxgiFactory7))))
	{
		Check(spDxgiFactory7->UnregisterAdaptersChangedEvent(m_adapterChangeRegistrationCookie));
		m_adapterChangeRegistrationCookie = 0;
		CloseHandle(m_adapterChangeEvent);
		m_adapterChangeEvent = NULL;
	}
#endif
	m_dxgiFactory.Reset();

#if defined(_DEBUG)
	{
		ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
#endif
}

void Graphics::GetGPUAdapter(UINT adapterIndex, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;
#ifdef USE_DXGI_1_6
	if (m_dxgiFactory->EnumAdapterByGpuPreference(adapterIndex, m_activeGpuPreference, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND)
#else
	if (m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
#endif
	{
		DXGI_ADAPTER_DESC1 desc;
		Check(adapter->GetDesc1(&desc));

		// Check to see if the adapter supports Direct3D 12.
		Check(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr));

		*ppAdapter = adapter.Detach();
	}
}

bool Graphics::QueryForAdapterEnumerationChanges()
{
     bool bChangeInAdapterEnumeration = false;
     if (m_adapterChangeEvent)
     {
     #ifdef USE_DXGI_1_6
     	// If QueryInterface for IDXGIFactory7 succeeded, then use RegisterAdaptersChangedEvent notifications.
     	DWORD waitResult = WaitForSingleObject(m_adapterChangeEvent, 0);
     	bChangeInAdapterEnumeration = (waitResult == WAIT_OBJECT_0);
     
     	if (bChangeInAdapterEnumeration)
     	{
     		// Before recreating the factory, unregister the adapter event
     		ComPtr<IDXGIFactory7> spDxgiFactory7;
     		if (SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&spDxgiFactory7))))
     		{
     			Check(spDxgiFactory7->UnregisterAdaptersChangedEvent(m_adapterChangeRegistrationCookie));
     			m_adapterChangeRegistrationCookie = 0;
     			CloseHandle(m_adapterChangeEvent);
     			m_adapterChangeEvent = NULL;
     		}
     	}
     #endif
     }
     else
     {
     	// Otherwise, IDXGIFactory7 doesn't exist, so continue using the polling solution of IsCurrent.
     	bChangeInAdapterEnumeration = !m_dxgiFactory->IsCurrent();
     }
     
     return bChangeInAdapterEnumeration;
}

HRESULT Graphics::ValidateActiveAdapter()
{
	EnumerateGPUadapters();

	if (!RetrieveAdapterIndex(&m_activeAdapter, m_activeAdapterLuid))
	{
		// The adapter is no longer available, default to 0.
		m_activeAdapter = 0;
		return DXGI_ERROR_DEVICE_RESET;
	}

	// Enforce adapter 0 being used, unless manual adapter selection is enabled.
	if (!m_manualAdapterSelection && m_activeAdapter != 0)
	{
		// A different adapter has become adapter 0, switch to it.
		m_activeAdapter = 0;
		return DXGI_ERROR_DEVICE_RESET;
	}

	return S_OK;
}

bool Graphics::RetrieveAdapterIndex(UINT* adapterIndex, LUID prevActiveAdapterLuid)
{
	for (UINT i = 0; i < m_gpuAdapterDescs.size(); i++)
	{
		if (memcmp(&m_gpuAdapterDescs[i].desc.AdapterLuid, &prevActiveAdapterLuid, sizeof(prevActiveAdapterLuid)) == 0)
		{
			*adapterIndex = i;
			return true;
		}
	}
	return false;
}

void Graphics::SelectAdapter(UINT index)
{
	if (index != m_activeAdapter && index < m_gpuAdapterDescs.size() && m_gpuAdapterDescs[index].supportsDx12FL11)
	{
		m_activeAdapter = index;
		RecreateD3Dresources();
	}
}

void Graphics::SelectGPUPreference(UINT index)
{
}

void Graphics::CalculateFrameStats()
{
}

void Graphics::WaitForGpu(ID3D12CommandQueue* pCommandQueue)
{
	// Schedule a Signal command in the queue.
	Check(pCommandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// Wait until the fence has been processed.
	Check(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}

void Graphics::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	Check(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		Check(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}
	m_scene->SetFrameIndex(m_frameIndex);

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}


void Graphics::Initiallize()
{
	LoadPipeline();
	LoadAssets();
	LoadSizeDependentResources();
}

void Graphics::Update()
{

	CalculateFrameStats();
	m_scene->Update();
}

void Graphics::Render()
{
	//if (m_windowVisible)
	{
		try
		{
			// Check for any adapter changes, such as a new adapter being available.
			if (QueryForAdapterEnumerationChanges())
			{
				// Dxgi factory needs to be recreated on a change.
				Check(CreateDXGIFactory2(m_dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));

#ifdef USE_DXGI_1_6
				ComPtr<IDXGIFactory7> spDxgiFactory7;
				if (SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&spDxgiFactory7))))
				{
					m_adapterChangeEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
					if (m_adapterChangeEvent == nullptr)
					{
						Check(HRESULT_FROM_WIN32(GetLastError()));
					}
					Check(spDxgiFactory7->RegisterAdaptersChangedEvent(m_adapterChangeEvent, &m_adapterChangeRegistrationCookie));
				}
#endif

				// Check if the application should switch to a different adapter.
				Check(ValidateActiveAdapter());
			}

			// UILayer will transition backbuffer to a present state.
			//bool bSetBackbufferReadyForPresent = !m_enableUI;
			m_scene->Render(m_commandQueue.Get(), false);


			// Present and update the frame index for the next frame.
			PIXBeginEvent(m_commandQueue.Get(), 0, L"Presenting to screen");
			// When using sync interval 0, it is recommended to always pass the tearing flag when it is supported.
			Check(m_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
			PIXEndEvent(m_commandQueue.Get());

			MoveToNextFrame();
		}
		catch (HrException& e)
		{
			if (e.Error() == DXGI_ERROR_DEVICE_REMOVED || e.Error() == DXGI_ERROR_DEVICE_RESET)
			{
				RecreateD3Dresources();
			}
			else
			{
				throw;
			}
		}
	}
}
