#pragma once

#include "SquidRoom.h"
#define SINGLETHREADED FALSE
#define GBufferCount 3

static const UINT NumNullSrvs = 2; // Null descriptors at the start of the heap.

// Currently the rendering code can only handle a single point light.
static const UINT NumLights = 1; // Keep this in sync with "ShadowsAndScenePass.hlsl".

// Command list submissions from main thread.
static const uint CommandListCount = 3;
static const uint CommandListPre = 0;
static const uint CommandListMid = 1;
static const uint CommandListPost = 2;

class FrameResource;

namespace SceneEnums
{
	namespace RenderPass {
		enum Value { Scene = 0, Shadow, Postprocess,Terrain, Count };
	}

	namespace DepthGenPass {
		enum Value { Scene = 0, Shadow, Count };
	}

	namespace RootSignature {
		enum { ShadowPass = 0, ScenePass, PostprocessPass, Count };
	};

	namespace VertexBuffer {
		enum Value { SceneGeometry = 0, ScreenQuad, Count };
	}

	namespace Timestamp {
		enum Value { ScenePass = 0, PostprocessPass, Count };
	}
}


struct LightState
{
	Vector4 position;
	Vector4 direction;
	Vector4 color;
	Vector4 falloff;
	Matrix view;
	Matrix projection;
};

struct SceneConstantBuffer
{
	Matrix model;
	Matrix view;
	Matrix projection;
	Vector4 ambientColor;
	BOOL sampleShadowMap;
	BOOL padding[3];        // Must be aligned to be made up of N Vector4s.
	LightState lights[NumLights];
	Vector4 viewport;
	Vector4 clipPlane;
};

struct PostprocessConstantBuffer
{
	Vector4 PerspectiveValues;
	Matrix ViewInv;

	Vector4 DirToLight;
	Vector3 DirLightColor;
	float Time;

	Matrix ToShadowSpace;
	Vector4 ToCascadeOffsetX;
	Vector4 ToCascadeOffsetY;
	Vector4 ToCascadeScale;
};

struct LightCamera
{
	Vector3 eye;
	Vector3 at;
	Vector3 up;
};

class Scene
{
public:
	Scene(uint frameCount);
	~Scene();
public:
	void Initiallize(ID3D12Device* device, ID3D12CommandQueue* directCommandQueue, ID3D12GraphicsCommandList* commandList, uint frameIndex);
	void LoadSizeDependentResources(ID3D12Device* device, ComPtr<ID3D12Resource>* renderTargets, uint width, uint height);
	void ReleaseSizeDependentResources();

	void SetFrameIndex(uint frameIndex);
	void ReleaseD3DObjects();
	void Update();
	void Render(ID3D12CommandQueue* commandQueue, bool setBackbufferReadyForPresent);
	static Scene* Get() { return app; }
private:
	static Scene* app;
	class SSAO* ssao;
	class Island11* island11;
	Camera* mainCamera;


	float m_fogDensity;

	float GetScenePassGPUTimeInMs() const;
	float GetPostprocessPassGPUTimeInMs() const;

protected:
	UINT m_frameCount;

	struct Vertex
	{
		Vector4 position;
	};

	vector<ID3D12CommandList*> m_batchSubmit;   // *2: shadowCommandLists, sceneCommandLists

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	// D3D objects.
	ComPtr<ID3D12GraphicsCommandList> m_commandLists[CommandListCount];
	vector<ComPtr<ID3D12GraphicsCommandList>> m_shadowCommandLists;
	vector<ComPtr<ID3D12GraphicsCommandList>> m_sceneCommandLists;
	vector<ComPtr<ID3D12GraphicsCommandList>> m_terrinCommandLists;
	std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
	std::vector<ComPtr<ID3D12Resource>> m_gbufferRenderTargets;

	
	ComPtr<ID3D12Resource> m_depthTextures[SceneEnums::DepthGenPass::Count];
	ComPtr<ID3D12RootSignature> m_rootSignatures[SceneEnums::RootSignature::Count];
	ComPtr<ID3D12PipelineState> m_pipelineStates[SceneEnums::RenderPass::Count];
	ComPtr<ID3D12Resource> m_vertexBuffers[SceneEnums::VertexBuffer::Count];
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[SceneEnums::VertexBuffer::Count];
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	vector< ComPtr<ID3D12Resource>>	m_textures;
	vector< ComPtr<ID3D12Resource>>	m_textureUploads;

	
	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_indexBufferUpload;
	ComPtr<ID3D12Resource> m_vertexBufferUpload;

	// Heap objects.

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
	ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
	UINT m_rtvDescriptorSize;
	UINT m_cbvSrvDescriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE m_depthDsvs[SceneEnums::DepthGenPass::Count];
	D3D12_CPU_DESCRIPTOR_HANDLE m_depthSrvCpuHandles[SceneEnums::DepthGenPass::Count];
	D3D12_GPU_DESCRIPTOR_HANDLE m_depthSrvGpuHandles[SceneEnums::DepthGenPass::Count];

	
	// Frame resources.
	std::vector<std::unique_ptr<FrameResource>> m_frameResources;
	FrameResource* m_pCurrentFrameResource;
	UINT m_frameIndex;
	SceneConstantBuffer m_shadowConstantBuffer; // Shadow copy.
	SceneConstantBuffer m_sceneConstantBuffer; // Shadow copy.
	PostprocessConstantBuffer m_postprocessConstantBuffer; // Shadow copy.

	
	LightState m_lights[NumLights];
	vector<LightCamera>lightCamera;
	static const float s_clearColor[4];

	// Window state
	bool m_windowVisible;
	bool m_windowedMode;


	
	vector< DXGI_FORMAT> m_gbufferRenderTargetFormats;
	
	uint threadCount;
	// Thread synchronization objects.
	struct ThreadParameter
	{
		int threadIndex;
	};
	vector<ThreadParameter> m_threadParameters;
	vector<HANDLE> m_workerBeginRenderFrame;

	vector<HANDLE> m_workerFinishShadowPass;

	vector<HANDLE> m_workerFinishedScenePass;

	vector<HANDLE> m_workerFinishTerrainPass;

	vector<HANDLE> m_threadHandles;
	
	void UpdateConstantBuffers(); // Updates the shadow copies of the constant buffers.
	void CommitConstantBuffers(); // Commits the shadows copies of the constant buffers to GPU-visible memory for the current frame.

	void DrawInScattering(ID3D12GraphicsCommandList* pCommandList, const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetHandle);


	void Get3DViewProjMatrices(Matrix* view, Matrix* proj, LightCamera lightCamera, float fovInDegrees, float screenWidth, float screenHeight);
	void WorkerThread(int threadIndex);
	void LoadContexts();

	void ShadowPass(ID3D12GraphicsCommandList* pCommandList, int threadIndex);
	void ScenePass(ID3D12GraphicsCommandList* pCommandList, int threadIndex);
	void TerrainPass(ID3D12GraphicsCommandList* pCommandList, int threadIndex);

	void PostprocessPass(ID3D12GraphicsCommandList* pCommandList);

	void BeginFrame();
	void MidFrame();
	void EndFrame(bool setBackbufferReadyForPresent);
	void InitializeCameraAndLights();

	void CreateDescriptorHeaps(ID3D12Device* pDevice);
	void CreateCommandLists(ID3D12Device* pDevice);
	void CreateRootSignatures(ID3D12Device* pDevice);
	void CreatePipelineStates(ID3D12Device* pDevice);
	void CreateFrameResources(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue);
	void CreateAssetResources(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList);
	void CreatePostprocessPassResources(ID3D12Device* pDevice);
	void CreateSamplers(ID3D12Device* pDevice);

	inline HRESULT CreateDepthStencilTexture2D(
		ID3D12Device* pDevice,
		UINT width,
		UINT height,
		DXGI_FORMAT typelessFormat,
		DXGI_FORMAT dsvFormat,
		DXGI_FORMAT srvFormat,
		ID3D12Resource** ppResource,
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDsvHandle,
		D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
		float initDepthValue = 1.0f,
		UINT8 initStencilValue = 0)
	{
		try
		{
			*ppResource = nullptr;

			CD3DX12_RESOURCE_DESC texDesc(
				D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				0,
				width,
				height,
				1,
				1,
				typelessFormat,
				1,
				0,
				D3D12_TEXTURE_LAYOUT_UNKNOWN,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

			Check(pDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				initState,
				&CD3DX12_CLEAR_VALUE(dsvFormat, initDepthValue, initStencilValue), // Performance tip: Tell the runtime at resource creation the desired clear value.
				IID_PPV_ARGS(ppResource)));

			// Create a depth stencil view (DSV).
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = dsvFormat;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
			pDevice->CreateDepthStencilView(*ppResource, &dsvDesc, cpuDsvHandle);

			// Create a shader resource view (SRV).
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = srvFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			pDevice->CreateShaderResourceView(*ppResource, &srvDesc, cpuSrvHandle);
		}
		catch (HrException& e)
		{
			SafeRelease(*ppResource);
			return e.Error();
		}
		return S_OK;
	}

	inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtvCpuHandle()
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	}

	
	// How much to scale each dimension in the world.
	inline float GetWorldScale() const
	{
		return 0.1f;
	}

	UINT GetNumRtvDescriptors() const
	{
		return m_frameCount+ GBufferCount;
	}

	UINT GetNumCbvSrvUavDescriptors() const
	{
		return NumNullSrvs + _countof(m_depthSrvCpuHandles) + m_textures.size()+ GBufferCount;
	}
};
