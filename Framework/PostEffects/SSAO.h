#pragma once

namespace SSAOEnums
{
	namespace CSPass {
		enum Value { DownScale = 0, SSAO,  Count };
	}


}
using namespace SSAOEnums;
class SSAO
{
public:
	explicit SSAO(ID3D12Device* device);
	~SSAO();

	SSAO(const SSAO&) = delete;
	SSAO& operator= (const SSAO&) = delete;

public:
	void Compute(ID3D12GraphicsCommandList* commnadList, const D3D12_GPU_DESCRIPTOR_HANDLE& DepthSRV, const D3D12_GPU_DESCRIPTOR_HANDLE& NormalsSRV);
	void Resize(const uint& width, const uint& height);
public:
	struct PARAMETERS_SSAO
	{
		float SSAOSampRadius;
		float Radius;
	}Params;
	PARAMETERS_SSAO* GetParams() { return &Params; }
private:

	void DownscaleDepth(ID3D12GraphicsCommandList* commnadList,  const D3D12_GPU_DESCRIPTOR_HANDLE& MiniDepth_UAV,const D3D12_GPU_DESCRIPTOR_HANDLE& DepthSRV, const D3D12_GPU_DESCRIPTOR_HANDLE& NormalsSRV);

	void ComputeSSAO(ID3D12GraphicsCommandList* commnadList, const D3D12_GPU_DESCRIPTOR_HANDLE& SSAO_UAV, const D3D12_GPU_DESCRIPTOR_HANDLE& MiniDepth_SRV);
	void CreateRootSignatures(ID3D12Device* pDevice);
	void CreatePipelineStates(ID3D12Device* pDevice);
private:
	uint downScaleGroups;
	UINT width;
	UINT height;

	//ID3D12Device* device;
	ComPtr<ID3D12RootSignature> rootSignatures[CSPass::Count];
	ComPtr<ID3D12PipelineState> pipelineStates[CSPass::Count];

};

