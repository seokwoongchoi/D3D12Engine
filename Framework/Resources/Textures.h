#pragma once

class Texture
{
public:
	friend class Textures;


public:
	Texture()
		:view(nullptr), texture(nullptr)//, metaData{}
	{

	}
	~Texture()=default;

	void Load(ID3D12Device* device, wstring file, D3DX12_IMAGE_LOAD_INFO* loadInfo = NULL,bool Include=false);
	inline operator  ID3D11ShaderResourceView*() { return view; }


	inline ID3D11ShaderResourceView* SRV() { return view; }
	wstring GetFile() { return file; }

	UINT GetWidth() { return imageInfo.Width; }
	UINT GetHeight() { return imageInfo.Height; }


	//DirectX::TexMetadata metaData;

	//void GetImageInfo(DirectX::TexMetadata* data)
	//{
	//	*data = metaData;
	//}

	//
	//ID3D11Texture2D* GetTexture();

private:
	wstring file;
	D3DX11_IMAGE_INFO imageInfo;
	ID3D11Texture2D* texture;
//	DirectX::TexMetadata metaData;
	ID3D11ShaderResourceView* view;

};

struct TextureDesc
{
	wstring file;
	uint width, height = 0;
	D3DX11_IMAGE_INFO imageInfo;
	ID3D11Texture2D* texture;
	//DirectX::TexMetadata metaData;
	ID3D11ShaderResourceView* view;

	bool operator==(const TextureDesc& desc)
	{
		bool b = true;
		b &= file == desc.file;
		b &= width == desc.width;
		b &= height == desc.height;

		return b;
	}
};

class Textures
{
public:
	friend class Texture;

public:
	static void Create();
	static void Delete();

private:
	static void Load(ID3D11Device* device, Texture* texture, D3DX11_IMAGE_LOAD_INFO* loadInfo = NULL);

private:
	static vector<TextureDesc> descs;
};


class TextureArray
{
public:
	explicit TextureArray(ID3D11Device* device,  vector<wstring>& names, UINT width = 256, UINT height = 256, UINT mipLevels = 1);
	~TextureArray();
	inline operator ID3D11ShaderResourceView*const*() { return &srv; }
	//ID3D11ShaderResourceView* SRV() { return srv; }

private:
	vector<ID3D11Texture2D *> CreateTextures(ID3D11Device* device, vector<wstring>& names, UINT width, UINT height, UINT mipLevels);

private:
	ID3D11ShaderResourceView* srv;
};