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

	compressPPM("Image/lena_ref_copy.ppm", "Image/lena_christ.dds");
	system("pause");
	return 0;
}