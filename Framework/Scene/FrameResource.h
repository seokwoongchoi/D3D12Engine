#pragma once
#include "Scene.h"
class FrameResource
{
public:
	// Performance tip: Minimize the number of command allocators and command lists.
	// Each open command list needs it's own command allocator.
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	vector<ComPtr<ID3D12CommandAllocator>> m_contextCommandAllocators;

	ComPtr<ID3D12Resource> m_constantBuffers[SceneEnums::RenderPass::Count];
	void* m_pConstantBuffersWO[SceneEnums::RenderPass::Count]; // WRITE-ONLY pointers to the constant buffers

	uint threadCount;

public:
	FrameResource(ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue,uint threadCount);
	virtual ~FrameResource();

	virtual void InitFrame();
	virtual void BeginFrame(ID3D12GraphicsCommandList* pCommandList);
	virtual void EndFrame(ID3D12GraphicsCommandList* pCommandList);

	inline D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferGPUVirtualAddress(SceneEnums::RenderPass::Value renderPass) const
	{
		return m_constantBuffers[renderPass]->GetGPUVirtualAddress();
	}
};

inline HRESULT CreateConstantBuffer(
	ID3D12Device* pDevice,
	UINT size,
	ID3D12Resource** ppResource,
	D3D12_CPU_DESCRIPTOR_HANDLE* pCpuCbvHandle = nullptr,
	D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ)
{
	try
	{
		*ppResource = nullptr;

		const UINT alignedSize = CalculateConstantBufferByteSize(size);
		Check(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(alignedSize),
			initState,
			nullptr,
			IID_PPV_ARGS(ppResource)));

		if (pCpuCbvHandle)
		{
			// Describe and create the shadow constant buffer view (CBV).
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.SizeInBytes = alignedSize;
			cbvDesc.BufferLocation = (*ppResource)->GetGPUVirtualAddress();
			pDevice->CreateConstantBufferView(&cbvDesc, *pCpuCbvHandle);
		}
	}
	catch (HrException& e)
	{
		SafeRelease(*ppResource);
		return e.Error();
	}
	return S_OK;
}