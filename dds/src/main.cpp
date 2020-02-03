#include <stdio.h>
#include <string>

#include "image_helper.h"
#include "ppm_helper.h"
#include "dds_helper.h"

int main()
{
	DXT1LoadTest("Image/lena_ref.dds", "Image/lena_ref_copy.ppm");
	system("pause");
	return 0;
}