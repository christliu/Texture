#include "dds_helper.h"
#include "ppm_helper.h"
#include <malloc.h>
#include <stdio.h>

extern int WriteColor32ToPPM(const char* dstfilename, unsigned int W, unsigned int H, Color32* data);

void BlockDXT1::decompress(Color32 color[16]) const
{
	Color32 palette[4];

	// Does bit expansion before interpolation.
	palette[0].b = (col0.b << 3) | (col0.b >> 2);
	palette[0].g = (col0.g << 2) | (col0.g >> 4);
	palette[0].r = (col0.r << 3) | (col0.r >> 2);
	palette[0].a = 0xFF;

	palette[1].r = (col1.r << 3) | (col1.r >> 2);
	palette[1].g = (col1.g << 2) | (col1.g >> 4);
	palette[1].b = (col1.b << 3) | (col1.b >> 2);
	palette[1].a = 0xFF;

	if (col0.u > col1.u)
	{
		// Four-color block: derive the other two colors.
		palette[2].r = (2 * palette[0].r + palette[1].r) / 3;
		palette[2].g = (2 * palette[0].g + palette[1].g) / 3;
		palette[2].b = (2 * palette[0].b + palette[1].b) / 3;
		palette[2].a = 0xFF;

		palette[3].r = (2 * palette[1].r + palette[0].r) / 3;
		palette[3].g = (2 * palette[1].g + palette[0].g) / 3;
		palette[3].b = (2 * palette[1].b + palette[0].b) / 3;
		palette[3].a = 0xFF;
	}
	else
	{
		// Three-color block: derive the other color.
		palette[2].r = (palette[0].r + palette[1].r) / 2;
		palette[2].g = (palette[0].g + palette[1].g) / 2;
		palette[2].b = (palette[0].b + palette[1].b) / 2;
		palette[2].a = 0xFF;

		palette[3].r = 0x00;
		palette[3].g = 0x00;
		palette[3].b = 0x00;
		palette[3].a = 0x00;
	}

	for (int i = 0; i < 16; i++)
	{
		color[i] = palette[(indices >> (2 * i)) & 0x3];
	}
}

int LoadDXT1ToColor32(const char* ddsfile, Color32** img, uint16_t *W, uint16_t *H)
{
	FILE* fp = fopen(ddsfile, "rb");
	if (fp == NULL)
		return -1;
	DDSHeader *srcHeader = (DDSHeader*)malloc(sizeof(DDSHeader));
	fread(srcHeader, sizeof(DDSHeader), 1, fp);

	printf("Header file loaded.\n");
	printf("Header fourcc: %04x size: %d flags: %d height: %d width: %d pitch: %d depth: %d mipmapcount: %d notused: %d",
		srcHeader->fourcc, srcHeader->size, srcHeader->flags, srcHeader->height, srcHeader->width, srcHeader->pitch,
		srcHeader->depth, srcHeader->mipmapcount, srcHeader->notused);

	Uint32 width = srcHeader->width, height = srcHeader->height;
	*W = width, *H = height;

	Uint32 blockSize = width / 4 * height / 4;
	BlockDXT1* blockdata = (BlockDXT1*)malloc(blockSize * 8); // 4096 * 4096 oversize?
	fread(blockdata, sizeof(BlockDXT1), blockSize, fp);

	fclose(fp);

	*img = (Color32*)malloc(width * height * 4);
	for (int row = 0; row < height / 4; row++)
	{
		for (int col = 0; col < width / 4; col++)
		{
			Uint32 offset = row * width / 4 + col;
			Color32 colors[16];
			(blockdata + offset)->decompress(colors);

			Uint32 pixelOffset = row * width / 4 * 16 + col * 4;  // every block start pixel offset

			int index = 0;

			for (int y = 0; y < 4; y++)
			{
				for (int x = 0; x < 4; x++)
				{
					colors[index].a = 0;
					(*img)[pixelOffset + y * width + x] = colors[index];
					index++;
				}
			}

		/*	for (int i = 0; i < 16; i++)
			{
				colors[i].a = 0;
				(*img)[offset * 16 + i] = colors[i];
			}*/
		}
	}
	return 0;
}

void DXT1LoadTest(const char* filename, const char* dstfile)
{
	Color32 *img = NULL;
	uint16_t w, h;

	if(LoadDXT1ToColor32(filename, &img, &w, &h) !=0)
		return;

	WriteColor32ToPPM(dstfile, w, h, img);
}