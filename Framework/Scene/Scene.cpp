#include "Framework.h"
#include "Scene.h"
#include "FrameResource.h"
#include "../Viewer/Freedom.h"
#include "PostEffects/SSAO.h"
#include "Environment/Island11_HLSL.h"
using namespace SceneEnums;

Scene* Scene::app = nullptr;
const float Scene::s_clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

Scene::Scene(uint frameCount)
	:
	m_frameCount(frameCount),
	m_fogDensity(0.04f),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, 0.0f, 0.0f),
	m_scissorRect(0, 0, 0L, 0L),
	m_rtvDescriptorSize(0),
	m_cbvSrvDescriptorSize(0),
	m_pCurrentFrameResource(nullptr)
	
{
	threadCount = std::thread::hardware_concurrency() - 1;
	m_threadParameters.resize(threadCount);
	m_workerBeginRenderFrame.resize(threadCount);
	m_workerFinishShadowPass.resize(threadCount);
	m_workerFinishedScenePass.resize(threadCount);
	m_workerFinishTerrainPass.resize(threadCount);

	m_threadHandles.resize(threadCount);
	m_shadowCommandLists.resize(threadCount);
	m_sceneCommandLists.resize(threadCount);
	m_terrinCommandLists.resize(threadCount);
	m_batchSubmit.resize((threadCount * 3 + CommandListCount));
	mainCamera = new Freedom();
	static_cast<Freedom*>(mainCamera)->Speed(10.0f, 0.1f);

	app = this;

	m_renderTargets.resize(m_frameCount, nullptr);
	m_gbufferRenderTargets.resize(GBufferCount,nullptr);
	
	m_gbufferRenderTargetFormats.emplace_back(DXGI_FORMAT_R10G10B10A2_UNORM);
	m_gbufferRenderTargetFormats.emplace_back(DXGI_FORMAT_R10G10B10A2_UNORM);
	m_gbufferRenderTargetFormats.emplace_back(DXGI_FORMAT_R16G16_UNORM);



	m_frameResources.resize(m_frameCount);
	m_textures.resize(_countof(SampleAssets::Textures));
	m_textureUploads.resize(_countof(SampleAssets::Textures));

	LoadContexts();
	InitializeCameraAndLights();
}

Scene::~Scene()
{
}

void Scene::Initiallize(ID3D12Device* device, ID3D12CommandQueue* directCommandQueue, ID3D12GraphicsCommandList* commandList, uint frameIndex)
{
	ssao = new SSAO(device);
	island11 = new Island11(device);
	CreateDescriptorHeaps(device);
	CreateRootSignatures(device);
	CreatePipelineStates(device);
	CreatePostprocessPassResources(device);
	CreateSamplers(device);
	CreateFrameResources(device, directCommandQueue);
	CreateCommandLists(device);

	CreateAssetResources(device, commandList);

	SetFrameIndex(frameIndex);

	
	
}

void Scene::LoadSizeDependentResources(ID3D12Device* device, ComPtr<ID3D12Resource>* renderTargets, uint width, uint height)
{
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = static_cast<LONG>(width);
	m_scissorRect.bottom = static_cast<LONG>(height);

	
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < m_frameCount; i++)
		{
			m_renderTargets[i] = renderTargets[i];
			device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvCpuHandle);
			rtvCpuHandle.Offset(1, m_rtvDescriptorSize);
			NAME_D3D12_OBJECT_INDEXED(m_renderTargets, i);
		}
	
		D3D12_RESOURCE_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.MipLevels = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Width = m_viewport.Width;
		resourceDesc.Height = m_viewport.Height;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ;

		D3D12_CLEAR_VALUE clearVal;
		
		clearVal.Color[0] = s_clearColor[0];
		clearVal.Color[1] = s_clearColor[1];
		clearVal.Color[2] = s_clearColor[2];
		clearVal.Color[3] = s_clearColor[3];



		const D3D12_CPU_DESCRIPTOR_HANDLE cbvSrvCPUHeapStart = m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
		const D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvGPUHeapStart = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCPUHandle(cbvSrvCPUHeapStart, NumNullSrvs + _countof(m_depthSrvCpuHandles) + m_textures.size() , m_cbvSrvDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGPUHandle(cbvSrvGPUHeapStart, NumNullSrvs + _countof(m_depthSrvGpuHandles) + m_textures.size() , m_cbvSrvDescriptorSize);

		for (uint i = 0; i < GBufferCount; i++)
		{
		
			resourceDesc.Format = m_gbufferRenderTargetFormats[i];
			clearVal.Format = m_gbufferRenderTargetFormats[i];
			Check(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&clearVal,
				IID_PPV_ARGS(&m_gbufferRenderTargets[i])));
			
			D3D12_RENDER_TARGET_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Texture2D.MipSlice = 0;
			desc.Texture2D.PlaneSlice = 0;

			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		

			device->CreateRenderTargetView(m_gbufferRenderTargets[i].Get(), &desc, rtvCpuHandle);
			rtvCpuHandle.Offset(1, m_rtvDescriptorSize);
		

		
			D3D12_SHADER_RESOURCE_VIEW_DESC descSRV;

			ZeroMemory(&descSRV, sizeof(descSRV));
			descSRV.Texture2D.MipLevels = 1;
			descSRV.Texture2D.MostDetailedMip = 0;
			descSRV.Format = m_gbufferRenderTargetFormats[i];
			descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;



			device->CreateShaderResourceView(m_gbufferRenderTargets[i].Get(), &descSRV, cbvSrvCPUHandle);

			cbvSrvCPUHandle.Offset(1,m_cbvSrvDescriptorSize);
			cbvSrvGPUHandle.Offset(1,m_cbvSrvDescriptorSize);
			NAME_D3D12_OBJECT_INDEXED(m_gbufferRenderTargets, i);

			
		}

	
		//{
		//	D3D12_RESOURCE_DESC resourceDesc;
		//	ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		//	resourceDesc.Alignment = 0;
		//	resourceDesc.SampleDesc.Count = 1;
		//	resourceDesc.SampleDesc.Quality = 0;
		//	resourceDesc.MipLevels = 1;
		//	resourceDesc.DepthOrArraySize = 1;
		//	resourceDesc.Width = m_viewport.Width;
		//	resourceDesc.Height = m_viewport.Height;
		//	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


		//	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		//	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		//	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		//	UAVDesc.Buffer.NumElements = 1;
		//	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

		//	

		//	device->CreateUnorderedAccessView(gBufferTextures[RTV_ORDER_QUAD], nullptr, &UAVDesc, srvHeap.handleCPU(srvHeapIndex));
		//}

	


	// Create the depth stencil views (DSVs).
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		const UINT dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		// Shadow depth resource.
		m_depthDsvs[DepthGenPass::Shadow] = dsvCpuHandle;
		Check(CreateDepthStencilTexture2D(device, width, height, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, &m_depthTextures[DepthGenPass::Shadow], m_depthDsvs[DepthGenPass::Shadow], m_depthSrvCpuHandles[DepthGenPass::Shadow]));
		NAME_D3D12_OBJECT(m_depthTextures[DepthGenPass::Shadow]);

		dsvCpuHandle.Offset(dsvDescriptorSize);

		// Scene depth resource.
		m_depthDsvs[DepthGenPass::Scene] = dsvCpuHandle;
		Check(CreateDepthStencilTexture2D(device, width, height, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, &m_depthTextures[DepthGenPass::Scene], m_depthDsvs[DepthGenPass::Scene], m_depthSrvCpuHandles[DepthGenPass::Scene]));
		NAME_D3D12_OBJECT(m_depthTextures[DepthGenPass::Scene]);
	}

}

void Scene::ReleaseSizeDependentResources()
{

	m_depthTextures[DepthGenPass::Shadow].Reset();
	m_depthTextures[DepthGenPass::Scene].Reset();

	for (UINT i = 0; i < m_frameCount; i++)
	{
		m_renderTargets[i].Reset();
		//m_gbufferRenderTargets[i].Reset();
	}
	
}

void Scene::SetFrameIndex(uint frameIndex)
{
	m_frameIndex = frameIndex;
	m_pCurrentFrameResource = m_frameResources[m_frameIndex].get();
}

void Scene::ReleaseD3DObjects()
{
	ResetComPtrArray(&m_renderTargets);
	ResetComPtrArray(&m_rootSignatures);
	ResetComPtrArray(&m_pipelineStates);
	ResetComPtrArray(&m_vertexBuffers);
	ResetComPtrArray(&m_rootSignatures);
	ResetComPtrArray(&m_textures);
	ResetComPtrArray(&m_textureUploads);
	m_indexBuffer.Reset();
	m_indexBufferUpload.Reset();
	m_vertexBufferUpload.Reset();

	m_rtvHeap.Reset();
	m_dsvHeap.Reset();
	m_cbvSrvHeap.Reset();
	m_samplerHeap.Reset();

	ResetUniquePtrArray(&m_frameResources);
	m_pCurrentFrameResource = nullptr;
}

void Scene::Update()
{
	mainCamera->Update();
	// Update camera and lights.
	const float angleChange = 2.0f * static_cast<float>(Time::Delta());

	//if (m_keyboardInput.leftArrowPressed)
	//	m_camera.RotateAroundYAxis(-angleChange);
	//if (m_keyboardInput.rightArrowPressed)
	//	m_camera.RotateAroundYAxis(angleChange);
	//if (m_keyboardInput.upArrowPressed)
	//	m_camera.RotatePitch(-angleChange);
	//if (m_keyboardInput.downArrowPressed)
	//	m_camera.RotatePitch(angleChange);

	//if (m_keyboardInput.animate)
	{
		for (int i = 0; i < NumLights; i++)
		{
			Matrix R;

			Matrix::RotationY(&R, angleChange);
			m_lights[i].position = static_cast<Vector3>(m_lights[i].position) * R;

			LightCamera light;
			light.eye = static_cast<Vector3>(m_lights[i].position);
			light.at = Vector3(0.0f, 15.0f, 0.0f);
			light.up = Vector3(0.0f, 1.0f, 0.0f);
			switch (i)
			{
			case 4: light.up = Vector3(0.0f, 0.0f, -1.0f); break;
			case 5: light.up = Vector3(0.0f, 0.0f, 1.0f); break;
			}

			Get3DViewProjMatrices(&m_lights[i].view, &m_lights[i].projection, light, 90.0f, m_viewport.Width, m_viewport.Height);

		}
	}

	// Update and commmit constant buffers.
	UpdateConstantBuffers();
	CommitConstantBuffers();
}

void Scene::Render(ID3D12CommandQueue* commandQueue, bool setBackbufferReadyForPresent)
{
	BeginFrame();
	// Shadow generation and scene render passes
#if SINGLETHREADED
	for (int i = 0; i < threadCount; i++)
	{
		WorkerThread(i);
	}
	MidFrame();
	EndFrame();
	pCommandQueue->ExecuteCommandLists(_countof(m_pCurrentFrameResource->m_batchSubmit), m_pCurrentFrameResource->m_batchSubmit);
#else
	for (int i = 0; i < threadCount; i++)
	{
		SetEvent(m_workerBeginRenderFrame[i]); // Tell each worker to start drawing.
	}

	MidFrame();
	EndFrame(setBackbufferReadyForPresent);

	WaitForMultipleObjects(threadCount, m_workerFinishShadowPass.data(), TRUE, INFINITE);


	commandQueue->ExecuteCommandLists(threadCount + 2, m_batchSubmit.data()); // Submit PRE, MID and shadows.
	//
	WaitForMultipleObjects(threadCount, m_workerFinishedScenePass.data(), TRUE, INFINITE);


	//// Submit remaining command lists.
	commandQueue->ExecuteCommandLists(m_batchSubmit.size()- threadCount - 2, m_batchSubmit.data()+ threadCount + 2);
#endif
}

float Scene::GetScenePassGPUTimeInMs() const
{
	return 0.0f;
}

float Scene::GetPostprocessPassGPUTimeInMs() const
{
	return 0.0f;
}

void Scene::UpdateConstantBuffers()
{
	// Scale down the world a bit.
	const float worldScale = GetWorldScale();

	Matrix::Scaling(&m_sceneConstantBuffer.model, worldScale, worldScale, worldScale);
	Matrix::Scaling(&m_shadowConstantBuffer.model, worldScale, worldScale, worldScale);


	m_sceneConstantBuffer.viewport = m_shadowConstantBuffer.viewport = { m_viewport.Width, m_viewport.Height, 0.0f, 0.0f };
	GlobalData::GetView(&m_sceneConstantBuffer.view);
	GlobalData::GetProj(&m_sceneConstantBuffer.projection);
	Get3DViewProjMatrices(&m_shadowConstantBuffer.view, &m_shadowConstantBuffer.projection, lightCamera[0], 90.0f, m_viewport.Width, m_viewport.Height);


	for (int i = 0; i < NumLights; i++)
	{
		memcpy(&m_sceneConstantBuffer.lights[i], &m_lights[i], sizeof(LightState));
		memcpy(&m_shadowConstantBuffer.lights[i], &m_lights[i], sizeof(LightState));
	}

	// The shadow pass won't sample the shadow map, but rather write to it.
	m_shadowConstantBuffer.sampleShadowMap = FALSE;

	// The scene pass samples the shadow map.
	m_sceneConstantBuffer.sampleShadowMap = TRUE;

	m_shadowConstantBuffer.ambientColor = m_sceneConstantBuffer.ambientColor = { 0.1f, 0.2f, 0.3f, 1.0f };

	m_postprocessConstantBuffer.DirToLight = Vector4(1, -1, 1,1);
	m_postprocessConstantBuffer.DirLightColor = Vector3(1,1,1);
	m_postprocessConstantBuffer.Time = Time::Delta();



	Matrix view = m_sceneConstantBuffer.view;

	Matrix::Inverse(&m_postprocessConstantBuffer.ViewInv, &view);


	const float fovAngleY = 90.0f;
	const float fovRadiansY = fovAngleY * 3.141592654f / 180.0f;
	Matrix proj;
	Matrix::PerspectiveFovLH(&proj, fovAngleY, m_viewport.Width / m_viewport.Height, 0.1, 1000.0f);

	m_postprocessConstantBuffer.PerspectiveValues.x = 1 / proj._11;
	m_postprocessConstantBuffer.PerspectiveValues.y = 1 / proj._22;
	m_postprocessConstantBuffer.PerspectiveValues.z = proj._43;
	m_postprocessConstantBuffer.PerspectiveValues.w = -proj._33;

	//Matrix::Inverse(&m_postprocessConstantBuffer.projInverse, &proj);

	//const float nearZ1 = 1.0f;

	//Matrix projAtNearZ1;
	//Matrix::PerspectiveFovLH(&projAtNearZ1, fovRadiansY, m_viewport.Width / m_viewport.Height, nearZ1, 1000.0f);

	//Matrix viewProjAtNearZ1 = view * projAtNearZ1;


	//Matrix::Inverse(&m_postprocessConstantBuffer.viewProjInverseAtNearZ1, &viewProjAtNearZ1);


}

void Scene::CommitConstantBuffers()
{
	memcpy(m_pCurrentFrameResource->m_pConstantBuffersWO[RenderPass::Scene], &m_sceneConstantBuffer, sizeof(m_sceneConstantBuffer));
	memcpy(m_pCurrentFrameResource->m_pConstantBuffersWO[RenderPass::Shadow], &m_shadowConstantBuffer, sizeof(m_shadowConstantBuffer));
	memcpy(m_pCurrentFrameResource->m_pConstantBuffersWO[RenderPass::Postprocess], &m_postprocessConstantBuffer, sizeof(m_postprocessConstantBuffer));
}

void Scene::DrawInScattering(ID3D12GraphicsCommandList* pCommandList, const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetHandle)
{
	// Set necessary state.

	pCommandList->SetGraphicsRootDescriptorTable(0, m_depthSrvGpuHandles[DepthGenPass::Scene]); // Set scene depth as an SRV.
	pCommandList->SetGraphicsRootConstantBufferView(1, m_pCurrentFrameResource->GetConstantBufferGPUVirtualAddress(RenderPass::Postprocess)); // Set postprocess constant buffer.

	const D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvHeapStart = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
	CD3DX12_GPU_DESCRIPTOR_HANDLE gbufferHandle(cbvSrvHeapStart, NumNullSrvs + _countof(m_depthSrvCpuHandles) + m_textures.size(), m_cbvSrvDescriptorSize);

	pCommandList->SetGraphicsRootDescriptorTable(2, gbufferHandle);

	pCommandList->SetGraphicsRootDescriptorTable(3, m_samplerHeap->GetGPUDescriptorHandleForHeapStart()); // Set samplers.

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferViews[VertexBuffer::ScreenQuad]);
	pCommandList->RSSetViewports(1, &m_viewport);
	pCommandList->RSSetScissorRects(1, &m_scissorRect);
	pCommandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, nullptr); // No depth stencil needed for in-scattering.

	// Draw.
	pCommandList->DrawInstanced(4, 1, 0, 0);
}

void Scene::Get3DViewProjMatrices(Matrix* view, Matrix* proj, LightCamera lightCamera, float fovInDegrees, float screenWidth, float screenHeight)
{
	float aspectRatio = (float)screenWidth / (float)screenHeight;
	float fovAngleY = fovInDegrees * 3.141592654f / 180.0f;

	if (aspectRatio < 1.0f)
	{
		fovAngleY /= aspectRatio;
	}

	Matrix::LookAtLH(view, lightCamera.eye, lightCamera.at, lightCamera.up);
	Matrix::PerspectiveFovLH(proj, fovAngleY, aspectRatio, 0.1f, 1000.0f);
}

void Scene::WorkerThread(int threadIndex)
{
	assert(threadIndex >= 0);
	assert(threadIndex < threadCount);

#if !SINGLETHREADED
	while (threadIndex >= 0 && threadIndex < threadCount)
	{
		// Wait for main thread to tell us to draw.
		WaitForSingleObject(m_workerBeginRenderFrame[threadIndex], INFINITE);

#endif

		//
		// Shadow pass
		//
		{
			ID3D12GraphicsCommandList* pShadowCommandList = m_shadowCommandLists[threadIndex].Get();

			// Reset the command list.
			Check(pShadowCommandList->Reset(m_pCurrentFrameResource->m_contextCommandAllocators[threadIndex].Get(), m_pipelineStates[SceneEnums::RenderPass::Shadow].Get()));
			PIXBeginEvent(pShadowCommandList, 0, L"Worker drawing shadow pass...");

			// Performance tip: Only set descriptor heaps if you need access to them.
			ShadowPass(pShadowCommandList, threadIndex);

			// Close the command list.
			PIXEndEvent(pShadowCommandList);
			Check(pShadowCommandList->Close());

#if !SINGLETHREADED
			// Tell main thread that we are done with the shadow pass.
			SetEvent(m_workerFinishShadowPass[threadIndex]);
#endif
		}



		//
		// terrain pass
		//
		{
			ID3D12GraphicsCommandList* pTerrainCommandList = m_terrinCommandLists[threadIndex].Get();

			// Reset the command list.
			Check(pTerrainCommandList->Reset(m_pCurrentFrameResource->m_contextCommandAllocators[threadIndex].Get(), m_pipelineStates[RenderPass::Scene].Get()));
			PIXBeginEvent(pTerrainCommandList, 0, L"Worker drawing terrin pass...");


			// Set descriptor heaps.
			//ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get(), m_samplerHeap.Get() };
			//pTerrainCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

			TerrainPass(pTerrainCommandList, threadIndex);



			Check(pTerrainCommandList->Close());

#if !SINGLETHREADED
			// Tell main thread that we are done with the scene pass.
			SetEvent(m_workerFinishTerrainPass[threadIndex]);
#endif
		}

		//
		// Scene pass
		//
		{
			ID3D12GraphicsCommandList* pSceneCommandList = m_sceneCommandLists[threadIndex].Get();
			
			// Reset the command list.
			Check(pSceneCommandList->Reset(m_pCurrentFrameResource->m_contextCommandAllocators[threadIndex].Get(), m_pipelineStates[RenderPass::Scene].Get()));
			PIXBeginEvent(pSceneCommandList, 0, L"Worker drawing scene pass...");


			// Set descriptor heaps.
			ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get(), m_samplerHeap.Get() };
			pSceneCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

			ScenePass(pSceneCommandList, threadIndex);

		
			
			// Close the command list.
			PIXEndEvent(pSceneCommandList);
			Check(pSceneCommandList->Close());

#if !SINGLETHREADED
			// Tell main thread that we are done with the scene pass.
			SetEvent(m_workerFinishedScenePass[threadIndex]);
#endif
		}


	}

	
}

void Scene::LoadContexts()
{
#if !SINGLETHREADED
	struct threadwrapper
	{
		static unsigned int WINAPI thunk(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			Scene::Get()->WorkerThread(parameter->threadIndex);
			return 0;
		}
	};

	for (int i = 0; i < threadCount; i++)
	{
		m_workerBeginRenderFrame[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishedScenePass[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishShadowPass[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishTerrainPass[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_threadParameters[i].threadIndex = i;

		m_threadHandles[i] = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr,
			0,
			threadwrapper::thunk,
			reinterpret_cast<LPVOID>(&m_threadParameters[i]),
			0,
			nullptr));

		assert(m_workerBeginRenderFrame[i] != NULL);
		assert(m_workerFinishedScenePass[i] != NULL);
		assert(m_workerFinishTerrainPass[i] != NULL);
		assert(m_threadHandles[i] != NULL);
	}
#endif
}

void Scene::ShadowPass(ID3D12GraphicsCommandList* pCommandList, int threadIndex)
{
	// Set necessary state.
	pCommandList->SetGraphicsRootSignature(m_rootSignatures[RootSignature::ShadowPass].Get());

	pCommandList->SetGraphicsRootConstantBufferView(0, m_pCurrentFrameResource->GetConstantBufferGPUVirtualAddress(RenderPass::Shadow));

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferViews[VertexBuffer::SceneGeometry]);
	pCommandList->IASetIndexBuffer(&m_indexBufferView);
	pCommandList->RSSetViewports(1, &m_viewport);
	pCommandList->RSSetScissorRects(1, &m_scissorRect);
	pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &m_depthDsvs[DepthGenPass::Shadow]); // No render target needed for the shadow pass.

	// Draw. Distribute objects over threads by drawing only 1/threadCount 
	// objects per worker (i.e. every object such that objectnum % 
	// threadCount == threadIndex).
	for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += threadCount)
	{
		const SampleAssets::DrawParameters& drawArgs = SampleAssets::Draws[j];
		pCommandList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);

	}
}

void Scene::ScenePass(ID3D12GraphicsCommandList* pCommandList, int threadIndex)
{
	// Set necessary state.
	pCommandList->SetGraphicsRootSignature(m_rootSignatures[RootSignature::ScenePass].Get());

	pCommandList->SetGraphicsRootConstantBufferView(1, m_pCurrentFrameResource->GetConstantBufferGPUVirtualAddress(RenderPass::Scene)); // Set scene constant buffer.
	pCommandList->SetGraphicsRootDescriptorTable(2, m_depthSrvGpuHandles[DepthGenPass::Shadow]); // Set the shadow texture as an SRV.
	pCommandList->SetGraphicsRootDescriptorTable(3, m_samplerHeap->GetGPUDescriptorHandleForHeapStart()); // Set samplers.

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferViews[VertexBuffer::SceneGeometry]);
	pCommandList->IASetIndexBuffer(&m_indexBufferView);
	pCommandList->RSSetViewports(1, &m_viewport);
	pCommandList->RSSetScissorRects(1, &m_scissorRect);

	//

	
	
				 
	pCommandList->OMSetRenderTargets(GBufferCount, &CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameCount , m_rtvDescriptorSize), true, &m_depthDsvs[DepthGenPass::Scene]);


	//pCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferRtvCpuHandle(), FALSE, &m_depthDsvs[DepthGenPass::Scene]);
	// Draw.
	const D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvHeapStart = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
	uint count = _countof(SampleAssets::Draws);
	for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += threadCount)
	{
		const SampleAssets::DrawParameters& drawArgs = SampleAssets::Draws[j];

		// Set the diffuse and normal textures for the current object.
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(cbvSrvHeapStart, NumNullSrvs + _countof(m_depthSrvCpuHandles) + drawArgs.DiffuseTextureIndex, m_cbvSrvDescriptorSize);
		pCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);

		pCommandList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
	}
}

void Scene::TerrainPass(ID3D12GraphicsCommandList* pCommandList, int threadIndex)
{

	island11->Test(threadIndex);
}

void Scene::PostprocessPass(ID3D12GraphicsCommandList* pCommandList)
{
	



	// Set necessary state.
	pCommandList->SetGraphicsRootSignature(m_rootSignatures[RootSignature::PostprocessPass].Get());
	pCommandList->SetPipelineState(m_pipelineStates[RenderPass::Postprocess].Get());

	DrawInScattering(pCommandList, GetCurrentBackBufferRtvCpuHandle());
}

void Scene::BeginFrame()
{
	m_pCurrentFrameResource->InitFrame();

	ID3D12GraphicsCommandList* pCommandListPre = m_commandLists[CommandListPre].Get();

	// Reset the command list.
	Check(pCommandListPre->Reset(m_pCurrentFrameResource->m_commandAllocator.Get(), nullptr));

	m_pCurrentFrameResource->BeginFrame(pCommandListPre);

	// Transition back-buffer to a writable state for rendering.
	pCommandListPre->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
	// Clear the render target and depth stencils.
	


	pCommandListPre->ClearRenderTargetView(GetCurrentBackBufferRtvCpuHandle(), s_clearColor, 0, nullptr);
	for (uint i = 0; i < GBufferCount; i++)
	{
		pCommandListPre->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameCount + i, m_rtvDescriptorSize), s_clearColor, 0, nullptr);
	}
	pCommandListPre->ClearDepthStencilView(m_depthDsvs[DepthGenPass::Shadow], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	pCommandListPre->ClearDepthStencilView(m_depthDsvs[DepthGenPass::Scene], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	
	// Close the command list.
	Check(pCommandListPre->Close());
}

void Scene::MidFrame()
{

	ID3D12GraphicsCommandList* pCommandListMid = m_commandLists[CommandListMid].Get();

	// Reset the command list.
	Check(pCommandListMid->Reset(m_pCurrentFrameResource->m_commandAllocator.Get(), nullptr));

	// Transition our shadow map to a readable state for scene rendering.
	pCommandListMid->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthTextures[DepthGenPass::Shadow].Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	

	// Close the command list.
	Check(pCommandListMid->Close());
}

void Scene::EndFrame(bool setBackbufferReadyForPresent)
{

	ID3D12GraphicsCommandList* pCommandListPost = m_commandLists[CommandListPost].Get();

	// Reset the command list.
	Check(pCommandListPost->Reset(m_pCurrentFrameResource->m_commandAllocator.Get(), nullptr));

	// Set descriptor heaps.
	ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get(), m_samplerHeap.Get() };
	pCommandListPost->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Transition scene depth to a readable state for post-processing.
	{
		D3D12_RESOURCE_BARRIER barriers[1 + GBufferCount];
		barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_depthTextures[DepthGenPass::Scene].Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		for (uint i = 1; i < GBufferCount+1; i++)
		{
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferRenderTargets[i-1].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
		}
		
		pCommandListPost->ResourceBarrier(_countof(barriers), barriers);
	}
	

	PIXBeginEvent(pCommandListPost, 0, L"Postprocess pass");
	//m_pCurrentFrameResource->m_gpuTimer.Start(pCommandListPost, Timestamp::PostprocessPass);
	PostprocessPass(pCommandListPost);
	//m_pCurrentFrameResource->m_gpuTimer.Stop(pCommandListPost, Timestamp::PostprocessPass);
	PIXEndEvent(pCommandListPost);

	// Transition depth buffers back to a writable state for the next frame
	// and conditionally indicate that the back buffer will now be used to present.
    // Performance tip: Batch resource barriers into as few API calls as possible to minimize the amount of work the GPU does.
	
	
	D3D12_RESOURCE_BARRIER barriers[3+ GBufferCount];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_depthTextures[DepthGenPass::Shadow].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_depthTextures[DepthGenPass::Scene].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	
	for (int i = 3; i < 3 + GBufferCount; i++)
		barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_gbufferRenderTargets[i - 3].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandListPost->ResourceBarrier(_countof(barriers), barriers);


	

	m_pCurrentFrameResource->EndFrame(pCommandListPost);

	// Close the command list.
	Check(pCommandListPost->Close());
}

void Scene::InitializeCameraAndLights()
{
	// Create lights.
	{
		for (int i = 0; i < NumLights; i++)
		{
			LightCamera light;
			switch (i)
			{
			case 0: m_lights[0].direction = { 0.0, 0.0f, 1.0f, 0.0f }; break;   // +z
#if NumLights > 1
			case 1: m_lights[1].direction = { 1.0, 0.0f, 0.0f, 0.0f }; break;   // +x
			case 2: m_lights[2].direction = { 0.0, 0.0f, -1.0f, 0.0f }; break;  // -z
			case 3: m_lights[3].direction = { -1.0, 0.0f, 0.0f, 0.0f }; break;  // -x
			case 4: m_lights[4].direction = { 0.0, 1.0f, 0.0f, 0.0f }; break;   // +y
			case 5: m_lights[5].direction = { 0.0, -1.0f, 0.0f, 0.0f }; break;  // -y
#endif
			}
			m_lights[i].position = { 0.0f, 15.0f, -30.0f, 1.0f };
			m_lights[i].falloff = { 120.0f, 1.0f, 0.0f, 1.0f };
			m_lights[i].color = { 0.7f, 0.7f, 0.7f, 1.0f };

			light.eye = static_cast<Vector3>(m_lights[i].position);
			light.at = light.eye + static_cast<Vector3>(m_lights[i].direction);
			light.up = Vector3(0.0f, 1.0f, 0.0f);
			switch (i)
			{
			case 4: light.up = Vector3(0.0f, 0.0f, -1.0f); break;
			case 5: light.up = Vector3(0.0f, 0.0f, 1.0f); break;
			}

			lightCamera.push_back(light);
		}
	}
}

void Scene::CreateDescriptorHeaps(ID3D12Device* device)
{
	

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = GetNumRtvDescriptors();
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Check(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
	

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = _countof(m_depthDsvs);
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Check(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
	cbvSrvHeapDesc.NumDescriptors = GetNumCbvSrvUavDescriptors();
	cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Check(device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
	
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};

	samplerHeapDesc.NumDescriptors = 2;
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Check(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));
	

	// Get descriptor sizes for the current device.
	m_cbvSrvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Scene::CreateCommandLists(ID3D12Device* device)
{
	ID3D12CommandAllocator* commandAllocator = m_frameResources[0]->m_commandAllocator.Get();
	for (uint i = 0; i < CommandListCount; i++)
	{
		Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&m_commandLists[i])));
	
		Check(m_commandLists[i]->Close());
	}

	for (uint i = 0; i < threadCount; i++)
	{
		Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,commandAllocator, nullptr, IID_PPV_ARGS(&m_shadowCommandLists[i])));
		Check(m_shadowCommandLists[i]->Close());

		Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&m_sceneCommandLists[i])));
		Check(m_sceneCommandLists[i]->Close());


		Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&m_terrinCommandLists[i])));
		Check(m_terrinCommandLists[i]->Close());
	}

	
	
	const uint batchSize = m_sceneCommandLists.size() + m_shadowCommandLists.size()+ m_terrinCommandLists.size() + CommandListCount;
	m_batchSubmit[0] = m_commandLists[CommandListPre].Get();
	memcpy(m_batchSubmit.data() + 1, m_shadowCommandLists.data(), m_shadowCommandLists.size() * sizeof(ID3D12CommandList*));
	m_batchSubmit[m_shadowCommandLists.size() + 1] = m_commandLists[CommandListMid].Get();
	memcpy(m_batchSubmit.data() + m_shadowCommandLists.size() + 2, m_terrinCommandLists.data(), m_terrinCommandLists.size() * sizeof(ID3D12CommandList*));
	memcpy(m_batchSubmit.data() + m_shadowCommandLists.size() + 2+ m_terrinCommandLists.size(), m_sceneCommandLists.data(), m_sceneCommandLists.size() * sizeof(ID3D12CommandList*));
	m_batchSubmit[batchSize - 1] = m_commandLists[CommandListPost].Get();
}

void Scene::CreateRootSignatures(ID3D12Device* device)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create a root signature for the shadow pass.
	{
		CD3DX12_ROOT_PARAMETER1 rootParameters[1]; // Performance tip: Order root parameters from most frequently accessed to least frequently accessed.
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX); // 1 frequently changed constant buffer.

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | // Performance tip: Limit root signature access when possible.
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		Check(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		Check(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatures[RootSignature::ShadowPass])));
		NAME_D3D12_OBJECT(m_rootSignatures[RootSignature::ShadowPass]);
	}

	// Create a root signature for the scene pass.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[4]; // Performance tip: Order root parameters from most frequently accessed to least frequently accessed.
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // 2 frequently changed diffuse + normal textures - starting in register t0.
		rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL); // 1 frequently changed constant buffer.
		rootParameters[2].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL); // 1 infrequently changed shadow texture - starting in register t2.
		rootParameters[3].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL); // 2 static samplers.

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | // Performance tip: Limit root signature access when possible.
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		Check(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		Check(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatures[RootSignature::ScenePass])));
		NAME_D3D12_OBJECT(m_rootSignatures[RootSignature::ScenePass]);
	}

	// Create a root signature for the post-process pass.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBufferCount, 1);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[4]; // Performance tip: Order root parameters from most frequently accessed to least frequently accessed.
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // 2 frequently changed diffuse + normal textures - starting in register t0.
		rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL); // 1 frequently changed constant buffer.
		rootParameters[2].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL); // 1 infrequently changed shadow texture - starting in register t2.
		rootParameters[3].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL); // 2 static samplers.
	
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		Check(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		Check(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatures[RootSignature::PostprocessPass])));
		NAME_D3D12_OBJECT(m_rootSignatures[RootSignature::PostprocessPass]);
	}
}

void Scene::CreatePipelineStates(ID3D12Device* device)
{
	// Create the scene and shadow render pass pipeline state.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		vertexShader = CompileShader(L"../_Shaders/ShadowsAndScenePass.hlsl", nullptr, "VSMain", "vs_5_0");
		pixelShader = CompileShader(L"../_Shaders/ShadowsAndScenePass.hlsl", nullptr, "PSMain", "ps_5_0");

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
		inputLayoutDesc.pInputElementDescs = SampleAssets::StandardVertexDescription;
		inputLayoutDesc.NumElements = _countof(SampleAssets::StandardVertexDescription);

		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = TRUE;
		depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D12_STENCIL_OP_REPLACE, D3D12_STENCIL_OP_REPLACE, D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS };
		depthStencilDesc.FrontFace = stencilMarkOp;
		depthStencilDesc.BackFace = stencilMarkOp;

		// Describe and create the PSO for rendering the scene.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = inputLayoutDesc;
		psoDesc.pRootSignature = m_rootSignatures[RootSignature::ScenePass].Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets =GBufferCount;
		
		for(uint i=0;i<GBufferCount;i++)
		psoDesc.RTVFormats[i] = m_gbufferRenderTargetFormats[i];


		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		Check(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStates[RenderPass::Scene])));
		NAME_D3D12_OBJECT(m_pipelineStates[RenderPass::Scene]);

		// Alter the description and create the PSO for rendering
		// the shadow map.  The shadow map does not use a pixel
		// shader or render targets.
		ZeroMemory(&psoDesc, sizeof(psoDesc));
		psoDesc.InputLayout = inputLayoutDesc;
		
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	
	

		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.pRootSignature = m_rootSignatures[RootSignature::ShadowPass].Get();
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0, 0);
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;

		Check(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStates[RenderPass::Shadow])));
		NAME_D3D12_OBJECT(m_pipelineStates[RenderPass::Shadow]);
	}

	// Create the postprocess pass pipeline state.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		vertexShader = CompileShader(L"../_Shaders/PostprocessPass.hlsl", nullptr, "VSMain", "vs_5_0");
		pixelShader = CompileShader(L"../_Shaders/PostprocessPass.hlsl", nullptr, "PSMain", "ps_5_0");



		// Define the vertex input layout. 
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = TRUE;
		depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D12_STENCIL_OP_REPLACE, D3D12_STENCIL_OP_REPLACE, D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS };
		depthStencilDesc.FrontFace = stencilMarkOp;
		depthStencilDesc.BackFace = stencilMarkOp;


		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignatures[RootSignature::PostprocessPass].Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
	    psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		Check(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStates[RenderPass::Postprocess])));
	}



}

void Scene::CreateFrameResources(ID3D12Device* device, ID3D12CommandQueue* pCommandQueue)
{
	for (UINT i = 0; i < m_frameCount; i++)
	{
		m_frameResources[i] = make_unique<FrameResource>(device, pCommandQueue,threadCount);
	}
	
}

void Scene::CreateAssetResources(ID3D12Device* device, ID3D12GraphicsCommandList* pCommandList)
{
	// Load scene assets.
	UINT fileSize = 0;
	UINT8* pAssetData;
	Check(ReadDataFromFile(SampleAssets::DataFileName, &pAssetData, &fileSize));

	// Create the vertex buffer.
	{
		Check(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffers[VertexBuffer::SceneGeometry])));
		NAME_D3D12_OBJECT(m_vertexBuffers[VertexBuffer::SceneGeometry]);

		Check(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBufferUpload)));

		// Copy data to the upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = pAssetData + SampleAssets::VertexDataOffset;
		vertexData.RowPitch = SampleAssets::VertexDataSize;
		vertexData.SlicePitch = vertexData.RowPitch;



		UpdateSubresources<1>(pCommandList, m_vertexBuffers[VertexBuffer::SceneGeometry].Get(), m_vertexBufferUpload.Get(), 0, 0, 1, &vertexData);

		// Performance tip: You can avoid some resource barriers by relying on resource state promotion and decay.
		// Resources accessed on a copy queue will decay back to the COMMON after ExecuteCommandLists()
		// completes on the GPU. Search online for "D3D12 Implicit State Transitions" for more details. 



		// Initialize the vertex buffer view.
		m_vertexBufferViews[VertexBuffer::SceneGeometry].BufferLocation = m_vertexBuffers[VertexBuffer::SceneGeometry]->GetGPUVirtualAddress();
		m_vertexBufferViews[VertexBuffer::SceneGeometry].SizeInBytes = SampleAssets::VertexDataSize;
		m_vertexBufferViews[VertexBuffer::SceneGeometry].StrideInBytes = SampleAssets::StandardVertexStride;
	}

	// Create the index buffer.
	{
		Check(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));
		NAME_D3D12_OBJECT(m_indexBuffer);

		Check(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBufferUpload)));

		// Copy data to the upload heap and then schedule a copy 
		// from the upload heap to the index buffer.
		D3D12_SUBRESOURCE_DATA indexData = {};
		indexData.pData = pAssetData + SampleAssets::IndexDataOffset;
		indexData.RowPitch = SampleAssets::IndexDataSize;
		indexData.SlicePitch = indexData.RowPitch;

		PIXBeginEvent(pCommandList, 0, L"Copy index buffer data to default resource...");

		UpdateSubresources<1>(pCommandList, m_indexBuffer.Get(), m_indexBufferUpload.Get(), 0, 0, 1, &indexData);

		// Performance tip: You can avoid some resource barriers by relying on resource state promotion and decay.
		// Resources accessed on a copy queue will decay back to the COMMON after ExecuteCommandLists()
		// completes on the GPU. Search online for "D3D12 Implicit State Transitions" for more details. 

		PIXEndEvent(pCommandList);

		// Initialize the index buffer view.
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.SizeInBytes = SampleAssets::IndexDataSize;
		m_indexBufferView.Format = SampleAssets::StandardIndexFormat;
	}

	// Create shader resources.
	{
		// Get a handle to the start of the descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

		{
			// Describe and create 2 null SRVs. Null descriptors are needed in order 
			// to achieve the effect of an "unbound" resource.
			D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
			nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			nullSrvDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			nullSrvDesc.Texture2D.MipLevels = 1;
			nullSrvDesc.Texture2D.MostDetailedMip = 0;
			nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			for (UINT i = 0; i < NumNullSrvs; i++)
			{
				device->CreateShaderResourceView(nullptr, &nullSrvDesc, cbvSrvCpuHandle);
				cbvSrvCpuHandle.Offset(m_cbvSrvDescriptorSize);
				cbvSrvGpuHandle.Offset(m_cbvSrvDescriptorSize);
			}
		}
		
		// Save the descriptor handles for the depth buffer views.
		for (UINT i = 0; i < _countof(m_depthSrvCpuHandles); i++)
		{
			m_depthSrvCpuHandles[i] = cbvSrvCpuHandle;
			m_depthSrvGpuHandles[i] = cbvSrvGpuHandle;
			cbvSrvCpuHandle.Offset(m_cbvSrvDescriptorSize);
			cbvSrvGpuHandle.Offset(m_cbvSrvDescriptorSize);
		}

		// Create each texture and SRV descriptor.
		PIXBeginEvent(pCommandList, 0, L"Copy diffuse and normal texture data to default resources...");
		for (UINT i = 0; i < _countof(SampleAssets::Textures); i++)
		{
			
			// Describe and create a Texture2D.
			const SampleAssets::TextureResource& tex = SampleAssets::Textures[i];
			CD3DX12_RESOURCE_DESC texDesc(
				D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				0,
				tex.Width,
				tex.Height,
				1,
				static_cast<UINT16>(tex.MipLevels),
				tex.Format,
				1,
				0,
				D3D12_TEXTURE_LAYOUT_UNKNOWN,
				D3D12_RESOURCE_FLAG_NONE);

			Check(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_textures[i])));
			NAME_D3D12_OBJECT_INDEXED(m_textures, i);

			{
				const UINT subresourceCount = texDesc.DepthOrArraySize * texDesc.MipLevels;
				UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textures[i].Get(), 0, subresourceCount);
				Check(device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_textureUploads[i])));

				// Copy data to the intermediate upload heap and then schedule a copy
				// from the upload heap to the Texture2D.
				D3D12_SUBRESOURCE_DATA textureData = {};
				textureData.pData = pAssetData + tex.Data->Offset;
				textureData.RowPitch = tex.Data->Pitch;
				textureData.SlicePitch = tex.Data->Size;

				UpdateSubresources(pCommandList, m_textures[i].Get(), m_textureUploads[i].Get(), 0, 0, subresourceCount, &textureData);

				// Performance tip: You can avoid some resource barriers by relying on resource state promotion and decay.
				// Resources accessed on a copy queue will decay back to the COMMON after ExecuteCommandLists()
				// completes on the GPU. Search online for "D3D12 Implicit State Transitions" for more details. 
			}

			// Describe and create an SRV.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = tex.Format;
			srvDesc.Texture2D.MipLevels = tex.MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			device->CreateShaderResourceView(m_textures[i].Get(), &srvDesc, cbvSrvCpuHandle);
			cbvSrvCpuHandle.Offset(m_cbvSrvDescriptorSize);
			cbvSrvGpuHandle.Offset(m_cbvSrvDescriptorSize);
		}





		
		PIXEndEvent(pCommandList);
	}

	free(pAssetData);
}

void Scene::CreatePostprocessPassResources(ID3D12Device* device)
{

	// Create the vertex buffer.
	//// Define the screen space quad geometry.
	//Vertex triangleVertices[] =
	//{
	//	{ { -1.0f, -1.0f, 0.0f, 1.0f } },
	//	{ { -1.0f, 1.0f, 0.0f, 1.0f } },
	//	{ { 1.0f, -1.0f, 0.0f, 1.0f } },
	//	{ { 1.0f, 1.0f, 0.0f, 1.0f } },
	//};

	Vertex triangleVertices[] =
	{
		{ { -1.0f, 1.0f, 0.0f, 1.0f } },
		{ { 1.0f, 1.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f, 1.0f } },
		{ { 1.0f, -1.0f, 0.0f, 1.0f } },
	};
	const UINT vertexBufferSize = sizeof(triangleVertices);

	// Note: using upload heaps to transfer static data like vert buffers is not 
	// recommended. Every time the GPU needs it, the upload heap will be marshalled 
	// over. Please read up on Default Heap usage. An upload heap is used here for 
	// code simplicity and because there are very few verts to actually transfer.
	Check(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffers[VertexBuffer::ScreenQuad])));
	NAME_D3D12_OBJECT(m_vertexBuffers[VertexBuffer::ScreenQuad]);

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin;
	const CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	Check(m_vertexBuffers[VertexBuffer::ScreenQuad]->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
	m_vertexBuffers[VertexBuffer::ScreenQuad]->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_vertexBufferViews[VertexBuffer::ScreenQuad].BufferLocation = m_vertexBuffers[VertexBuffer::ScreenQuad]->GetGPUVirtualAddress();
	m_vertexBufferViews[VertexBuffer::ScreenQuad].StrideInBytes = sizeof(Vertex);
	m_vertexBufferViews[VertexBuffer::ScreenQuad].SizeInBytes = vertexBufferSize;
}

void Scene::CreateSamplers(ID3D12Device* device)
{
	// Get the sampler descriptor size for the current device.
	const UINT samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Get a handle to the start of the descriptor heap.
	CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(m_samplerHeap->GetCPUDescriptorHandleForHeapStart());

	// Describe and create the point clamping sampler, which is 
	// used for the shadow map.
	D3D12_SAMPLER_DESC clampSamplerDesc = {};
	clampSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	clampSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	clampSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	clampSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	clampSamplerDesc.MipLODBias = 0.0f;
	clampSamplerDesc.MaxAnisotropy = 1;
	clampSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	clampSamplerDesc.BorderColor[0] = clampSamplerDesc.BorderColor[1] = clampSamplerDesc.BorderColor[2] = clampSamplerDesc.BorderColor[3] = 0;
	clampSamplerDesc.MinLOD = 0;
	clampSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	device->CreateSampler(&clampSamplerDesc, samplerHandle);

	// Move the handle to the next slot in the descriptor heap.
	samplerHandle.Offset(samplerDescriptorSize);

	// Describe and create the wrapping sampler, which is used for 
	// sampling diffuse/normal maps.
	D3D12_SAMPLER_DESC wrapSamplerDesc = {};
	wrapSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	wrapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	wrapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	wrapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	wrapSamplerDesc.MinLOD = 0;
	wrapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	wrapSamplerDesc.MipLODBias = 0.0f;
	wrapSamplerDesc.MaxAnisotropy = 1;
	wrapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	wrapSamplerDesc.BorderColor[0] = wrapSamplerDesc.BorderColor[1] = wrapSamplerDesc.BorderColor[2] = wrapSamplerDesc.BorderColor[3] = 0;
	device->CreateSampler(&wrapSamplerDesc, samplerHandle);
}
