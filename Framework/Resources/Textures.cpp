#include "Framework.h"
#include "Textures.h"


//using namespace DirectX;
vector<TextureDesc> Textures::descs;



void Texture::Load(ID3D11Device* device, wstring file, D3DX11_IMAGE_LOAD_INFO * loadInfo, bool IsInclude)
{
	this->file = file;
	bool b = Path::IsRelativePath(file);
	wstring temp = L"../../_Textures/";
	if (IsInclude)
	{
		temp = L"../_Textures/";
	}
	if (b == true)
		this->file = temp + file;



	Textures::Load(device,this, loadInfo);
	
	String::Replace(&this->file, temp, L"");
}

void Textures::Create()
{
}

void Textures::Delete()
{
	for (TextureDesc desc : descs)
		SafeRelease(desc.view);
}

void Textures::Load(ID3D11Device* device,Texture * texture, D3DX11_IMAGE_LOAD_INFO * loadInfo)
{
	//TexMetadata metaData;
	//wstring ext = Path::GetExtension(texture->file);
	//if (ext == L"tga")
	//{
	//	Check(GetMetadataFromTGAFile(texture->file.c_str(), metaData));
	//	
	//}
	//else if (ext == L"dds")
	//{
	//	Check(GetMetadataFromDDSFile(texture->file.c_str(), DDS_FLAGS_NONE, metaData));
	//	
	//}
	//else if (ext == L"hdr")
	//{
	//	assert(false);
	//}
	//else
	//{
	//	Check(GetMetadataFromWICFile(texture->file.c_str(), WIC_FLAGS_NONE, metaData));
	//	
	//}

	//UINT width = metaData.width;
	//UINT height = metaData.height;

	//if (loadInfo != NULL)
	//{
	//	width = loadInfo->Width;
	//	height = loadInfo->Height;

	//	metaData.width = loadInfo->Width;
	//	metaData.height = loadInfo->Height;
	//}


	//TextureDesc desc;
	//desc.file = texture->file;
	//desc.width = width;
	//desc.height = height;

	//TextureDesc exist;
	//bool bExist = false;
	//for (TextureDesc temp : descs)
	//{
	//	if (desc == temp)
	//	{
	//		bExist = true;
	//		exist = temp;

	//		break;
	//	}
	//}

	//if (bExist == true)
	//{
	//	texture->metaData = exist.metaData;
	//	texture->view = exist.view;
	//}
	//else
	//{
	//	ScratchImage image;
	//	if (ext == L"tga")
	//	{
	//		Check(LoadFromTGAFile(texture->file.c_str(), &metaData, image));
	//	}
	//	else if (ext == L"dds")
	//	{
	//		Check(LoadFromDDSFile(texture->file.c_str(), DDS_FLAGS_NONE, &metaData, image));
	//	
	//	}
	//	else if (ext == L"hdr")
	//	{
	//		assert(false);
	//	}
	//	else
	//	{
	//		Check(LoadFromWICFile(texture->file.c_str(), WIC_FLAGS_NONE, &metaData, image));
	//		
	//	}

	//	ID3D11ShaderResourceView* view;

	//	Check(DirectX::CreateShaderResourceView(device, image.GetImages(), image.GetImageCount(), metaData, &view));
	//

	//	desc.file = texture->file;
	//	desc.width = metaData.width;
	//	desc.height = metaData.height;
	//	desc.view = view;
	//	desc.metaData = metaData;

	//	texture->view = view;
	//	texture->metaData = metaData;

	//	descs.push_back(desc);
	//}
	HRESULT hr;

	D3DX11_IMAGE_INFO imageInfo;
	
	hr = D3DX11GetImageInfoFromFile(texture->file.c_str(), NULL, &imageInfo, NULL);
	

	UINT width = imageInfo.Width;
	UINT height = imageInfo.Height;

	if (loadInfo != NULL)
	{
		width = loadInfo->Width;
		height = loadInfo->Height;

		imageInfo.Width = loadInfo->Width;
		imageInfo.Height = loadInfo->Height;
	}


	TextureDesc desc;
	desc.file = texture->file;
	desc.width = width;
	desc.height = height;

	TextureDesc exist;
	bool bExist = false;

	
	auto find = find_if(descs.begin(), descs.end(), [=, &desc](const TextureDesc& temp)
	{
		return desc == temp;

	});
	if (find != descs.end())
	{
		bExist = true;
		exist = *find;
	}
	/*for (TextureDesc temp : descs)
	{
		if (desc == temp)
		{
			bExist = true;
			exist = temp;

			break;
		}
	}*/

	
	if (bExist == true)
	{
		texture->imageInfo = exist.imageInfo;
		texture->view = exist.view;
	}
	else
	{
		wstring ext = Path::GetExtension(texture->file);

		
		{
			ID3D11ShaderResourceView* view;
			D3DX11CreateShaderResourceViewFromFile(device, texture->file.c_str(), NULL, NULL, &view, NULL);
			desc.file = texture->file;
			desc.width = imageInfo.Width;
			desc.height = imageInfo.Height;
			desc.view = view;
			desc.imageInfo = imageInfo;

			texture->view = view;
			texture->imageInfo = imageInfo;

			descs.push_back(desc);
		
		}

		
		
		
	
	}
}

TextureArray::TextureArray(ID3D11Device* device, vector<wstring>& names, UINT width, UINT height, UINT mipLevels)
	:srv(nullptr)
{
	for (UINT i = 0; i < names.size(); i++)
		names[i] = L"../../_Textures/" + names[i];

	vector<ID3D11Texture2D *> textures;
	textures = CreateTextures(device,names, width, height, mipLevels);


	D3D11_TEXTURE2D_DESC textureDesc;
	textures[0]->GetDesc(&textureDesc);

	ID3D11Texture2D* textureArray;
	//Texture2DArray
	{
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = textureDesc.Width;
		desc.Height = textureDesc.Height;
		desc.MipLevels = textureDesc.MipLevels;
		desc.ArraySize = static_cast<uint>(names.size());
		desc.Format = textureDesc.Format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		HRESULT hr = device->CreateTexture2D(&desc, NULL, &textureArray);
		Check(hr);
	}
	ID3D11DeviceContext* immediateContext;
	device->GetImmediateContext(&immediateContext);
	for (UINT i = 0; i < textures.size(); i++)
	{
		for (UINT level = 0; level < textureDesc.MipLevels; level++)
		{
			D3D11_MAPPED_SUBRESOURCE subResource;
			immediateContext->Map(textures[i], level, D3D11_MAP_READ, 0, &subResource);
			{
				immediateContext->UpdateSubresource(textureArray, D3D11CalcSubresource(level, i, textureDesc.MipLevels), NULL, subResource.pData, subResource.RowPitch, subResource.DepthPitch);
			}
			immediateContext->Unmap(textures[i], level);
		}
	}

	//Create File textures[0] -> test.png (for Test)
	//D3DX11SaveTextureToFile(D3D::GetDC(), textureArray, D3DX11_IFF_PNG, L"test.png");

	//SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		desc.Format = textureDesc.Format;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MostDetailedMip = 0;
		desc.Texture2DArray.MipLevels = textureDesc.MipLevels;
		desc.Texture2DArray.FirstArraySlice = 0;
		desc.Texture2DArray.ArraySize = static_cast<uint>(names.size());

		HRESULT hr = device->CreateShaderResourceView(textureArray, &desc, &srv);
		Check(hr);
	}

	for (ID3D11Texture2D* texture : textures)
		SafeRelease(texture);

	SafeRelease(textureArray);
}

TextureArray::~TextureArray()
{
	SafeRelease(srv);

}


vector<ID3D11Texture2D*> TextureArray::CreateTextures(ID3D11Device* device, vector<wstring>& names, UINT width, UINT height, UINT mipLevels)
{
	vector<ID3D11Texture2D *> returnTextures;
	return returnTextures;
//	vector<ID3D11Texture2D *> returnTextures;
//	returnTextures.resize(names.size());
//
//	for (UINT index = 0; index < returnTextures.size(); index++)
//	{
//		HRESULT hr;
//
//		TexMetadata metaData;
//		wstring ext = Path::GetExtension(names[index]);
//		if (ext == L"tga")
//		{
//			hr = GetMetadataFromTGAFile(names[index].c_str(), metaData);
//			Check(hr);
//		}
//		else if (ext == L"dds")
//		{
//			hr = GetMetadataFromDDSFile(names[index].c_str(), DDS_FLAGS_NONE, metaData);
//			Check(hr);
//		}
//		else if (ext == L"hdr")
//		{
//			assert(false);
//		}
//		else
//		{
//			hr = GetMetadataFromWICFile(names[index].c_str(), WIC_FLAGS_NONE, metaData);
//			Check(hr);
//		}
//
//		ScratchImage image;
//
//		if (ext == L"tga")
//		{
//			hr = LoadFromTGAFile(names[index].c_str(), &metaData, image);
//			Check(hr);
//		}
//		else if (ext == L"dds")
//		{
//			hr = LoadFromDDSFile(names[index].c_str(), DDS_FLAGS_NONE, &metaData, image);
//			Check(hr);
//		}
//		else if (ext == L"hdr")
//		{
//			assert(false);
//		}
//		else
//		{
//			hr = LoadFromWICFile(names[index].c_str(), WIC_FLAGS_NONE, &metaData, image);
//			Check(hr);
//		}
//
//		/*ScratchImage resizedImage;
//		if (image.GetImages()->width > width&&image.GetImages()->width > height)
//		{
//			hr = DirectX::Resize
//			(
//				image.GetImages(), image.GetImageCount(), image.GetMetadata(), width, height, TEX_FILTER_DEFAULT, resizedImage
//			);
//			Check(hr);
//		}
//		
//*/
//		if (mipLevels > 1)
//		{
//			ScratchImage mipmapedImage;
//			hr = DirectX::GenerateMipMaps
//			(
//				image.GetImages(), image.GetImageCount(), image.GetMetadata(), D3DX11_FILTER_NONE, mipLevels, mipmapedImage
//			);
//			Check(hr);
//
//			hr = DirectX::CreateTextureEx
//			(
//				device
//				, mipmapedImage.GetImages()
//				, mipmapedImage.GetImageCount()
//				, mipmapedImage.GetMetadata()
//				, D3D11_USAGE_STAGING
//				, 0
//				, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE
//				, 0
//				, false
//				, (ID3D11Resource **)&returnTextures[index]
//			);
//			Check(hr);
//
//			mipmapedImage.Release();
//		}
//		else
//		{
//			hr = DirectX::CreateTextureEx
//			(
//				device
//				, image.GetImages()
//				, image.GetImageCount()
//				, image.GetMetadata()
//				, D3D11_USAGE_STAGING
//				, 0
//				, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE
//				, 0
//				, false
//				, (ID3D11Resource **)&returnTextures[index]
//			);
//			Check(hr);
//		}
//
//		image.Release();
//		//resizedImage.Release();
//
//	}
//	return returnTextures;
}
