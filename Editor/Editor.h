#pragma once

struct FrameContext
{
	ID3D12CommandAllocator* CommandAllocator;
	UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;


static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = NULL;

static ID3D12GraphicsCommandList*   g_pd3dCommandList = NULL;
static ID3D12Fence*                 g_fence = NULL;
static HANDLE                       g_fenceEvent = NULL;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3*             g_pSwapChain = NULL;
static HANDLE                       g_hSwapChainWaitableObject = NULL;


// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd, class DXSample* sample);
void CleanupDeviceD3D();

void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();


class Editor final
{
	
public:
	Editor();
	~Editor();

	Editor(const Editor&) = delete;
	Editor& operator=(const Editor&) = delete;
	
	void Resize(const uint& width, const uint& height);
	void Update();
	void Render();

private:
	class Camera* mainCamera;
	class DXSample* sample;


	//// Data
	//  const int                     NUM_FRAMES_IN_FLIGHT = 3;
	//  FrameContext                 g_frameContext[3];
	//  UINT                         g_frameIndex = 0;

	//  int const                    NUM_BACK_BUFFERS = 3;
	//  ID3D12Device*                g_pd3dDevice = NULL;
	//  ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = NULL;
	//  ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = NULL;
	//  ID3D12CommandQueue*          g_pd3dCommandQueue = NULL;
	//  ID3D12GraphicsCommandList*   g_pd3dCommandList = NULL;
	//  ID3D12Fence*                 g_fence = NULL;
	//  HANDLE                       g_fenceEvent = NULL;
	//  UINT64                       g_fenceLastSignaledValue = 0;
	//  IDXGISwapChain3*             g_pSwapChain = NULL;
	//  HANDLE                       g_hSwapChainWaitableObject = NULL;
	//  ID3D12Resource*              g_mainRenderTargetResource[3] = {};
	//  D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[3] = {};

	
	  // Our state
	  bool show_demo_window = true;
	  bool show_another_window = false;
	  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	//// Forward declarations of helper functions
	//bool CreateDeviceD3D(HWND hWnd);
	//void CleanupDeviceD3D();
	//void CreateRenderTarget();
	//void CleanupRenderTarget();
	//void WaitForLastSubmittedFrame();
	//FrameContext* WaitForNextFrameResources();
	//void ResizeSwapChain(HWND hWnd, int width, int height);
};