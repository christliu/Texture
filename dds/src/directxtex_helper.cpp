#include "directxtex_helper.h"
#include <memory>
#include <wrl\wrappers\corewrappers.h>
using namespace DirectX;

void directInit()
{

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("error CoInitializeEx\n");
	}
#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
	Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
	if (FAILED(initialize))
	{
		printf("error \n");
	}
#else
	hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("error\n");
	}
#endif

}

void loaddds()
{
	TexMetadata info;
	ScratchImage img;
	LoadFromDDSFile(L"Image/lena_ref.dds", DDS_FLAGS_NONE, &info, img);
}

void loadPng(const wchar_t* filename)
{
	auto image = std::make_unique<ScratchImage>();
	HRESULT hr = LoadFromWICFile(filename, WIC_FLAGS_NONE, nullptr, *image);
	if (FAILED(hr))
	{
		printf("Error load %s", filename);
		return;
	}

	ScratchImage bcImage;
	hr = Compress(image->GetImages(), image->GetImageCount(),
		image->GetMetadata(), DXGI_FORMAT_BC1_UNORM,
		TEX_COMPRESS_DEFAULT, TEX_THRESHOLD_DEFAULT,
		bcImage);

	if (FAILED(hr))
	{
		printf("Compress Error\n");
		return;
	}

	hr = SaveToDDSFile(bcImage.GetImage(0, 0, 0), bcImage.GetImageCount(), bcImage.GetMetadata(), DDS_FLAGS_NONE, L"Image/copy.dds");
	if (FAILED(hr))
	{
		printf("error save to dds\n");
		return;
	}
}