#include <stdio.h>
#include <string>

#include "image_helper.h"
#include "ppm_helper.h"
#include "dds_helper.h"
#include "directxtex_helper.h"

int main()
{
	//DXT1LoadTest("Image/lena_ref.dds", "Image/lena_ref_copy.ppm");
	//loaddds();
	//copyDxt1("Image/lena_ref.dds", "Image/lena_ref_copy.dds");

	compressPPM("Image/lena_std.ppm", "Image/lena_christ.dds");
	//compressPPM("Image/red.ppm", "Image/red_christ.dds");
	int offset = 115;
	//computeError("Image/lena_christ.dds", "Image/lena_ref.dds");

	//directInit();
	//loadPng(L"Image/eye.png");

	system("pause");
	return 0;
}