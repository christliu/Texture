#include "directxtex_helper.h"
using namespace DirectX;

void loaddds()
{
	TexMetadata info;
	ScratchImage img;
	LoadFromDDSFile(L"Image/lena_ref.dds", DDS_FLAGS_NONE, &info, img);
}