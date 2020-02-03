#include <stdio.h>
#include <cstring>
#include <cassert>
#include "malloc.h"
#include "ppm_helper.h"

int LoadPPM(const char* filename, unsigned int* W, unsigned int* H, unsigned int* channels, uint8_t** data)
{
	FILE *fp = NULL;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("File %s not exists\n", filename);
		return -1;
	}

	// check header
	const unsigned int PGMHeaderSize = 0x40;
	char header[PGMHeaderSize];

	if (fgets(header, PGMHeaderSize, fp) == NULL) {
		printf("__LoadPPM() : reading PGM header returned NULL");
		return -1;
	}

	if (strncmp(header, "P5", 2) == 0) {
		*channels = 1;
	}
	else if (strncmp(header, "P6", 2) == 0) {
		*channels = 3;
	}
	else {
		printf("__LoadPPM() : File is not a PPM or PGM image");
		*channels = 0;
		return -1;
	}

	// parse header, read maxval, width and height
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int maxval = 0;
	unsigned int i = 0;

	while (i < 3) {
		if (fgets(header, PGMHeaderSize, fp) == NULL) {
			printf("__LoadPPM() : reading PGM header returned NULL\n");
			return -1;
		}

		if (header[0] == '#') {
			continue;
		}

		if (i == 0) {
			i += sscanf(header, "%u %u %u", &width, &height, &maxval);
		}
		else if (i == 1) {
			i += sscanf(header, "%u %u", &height, &maxval);
		}
		else if (i == 2) {
			i += sscanf(header, "%u", &maxval);
		}
	}

	// check if given handle for the data is initialized
	if (NULL != *data) {
		if (*W != width || *H != height) {
			printf("__LoadPPM() : Invalid image dimensions.");
		}
	}
	else {
		*data = (unsigned char *)malloc(sizeof(unsigned char) * width * height *
			*channels);
		*W = width;
		*H = height;
	}

	if (fread(*data, sizeof(unsigned char), width * height * *channels, fp) ==
		0) {
		printf("__LoadPPM() read data returned error.");
		return -1;
	}
	return 0;
}

int LoadAndFormatPPM(const char* filename, unsigned int *W, unsigned int *H, Color32** data)
{
	unsigned int Width, Height;
	unsigned int Channel;
	uint8_t* bytes = NULL;

	if(LoadPPM(filename, &Width, &Height, &Channel, &bytes) !=0)
		return -1;

	*W = Width;
	*H = Height;

	*data = (Color32*)malloc(Width * Height * sizeof(Color32));
	for (int i = 0; i < Width * Height; i ++)
	{
		Uint32 r, g, b;
		r = bytes[i * 3];
		g = bytes[i * 3 + 1];
		b = bytes[i * 3 + 2];

		Uint32 color = (0 << 24) | (r << 16) | (g << 8) | b;
		if (i == 16384)
		{
			printf("goi\n");
		}
		(*data)[i].u = color;
	}
	return 0;
}

int WriteColor32ToPPM(const char* dstfilename, unsigned int W, unsigned int H, Color32* data)
{
	FILE *fp = fopen(dstfilename, "w");
	if (fp == NULL)
		return -1;

	fprintf(fp, "P6\n");
	fprintf(fp, "%d %d\n", W, H);
	fprintf(fp, "%d\n", 255);

	int Count = W * H;

	for (int i = 0; i < Count; i++)
	{
		uint8_t r, g, b, a;
		r = (data[i].u & 0xff0000) >> 16;
		g = (data[i].u & 0xff00) >> 8;
		b = (data[i].u & 0xff);
		a = (data[i].u & 0xff000000) >> 24;
		assert(a == 0);
		fwrite(&r, sizeof(uint8_t), 1, fp);
		fwrite(&g, sizeof(uint8_t), 1, fp);
		fwrite(&b, sizeof(uint8_t), 1, fp);
	}
	fclose(fp);
	return W * H * sizeof(Color32);

}


int LoadPPMTest(const char* filename, const char* dstfile)
{
	unsigned int Width, Height;
	unsigned int Channel;

	Color32* color32;

	if(LoadAndFormatPPM(filename, &Width, &Height, &color32) != 0)
		return -1;
	printf("Load file %s to byte done..\n", filename);


	WriteColor32ToPPM(dstfile, Width, Height, color32);
}