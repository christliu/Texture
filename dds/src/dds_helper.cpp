#include "dds_helper.h"
#include "ppm_helper.h"
#include <malloc.h>
#include <stdio.h>
#include <cstring>

extern int WriteColor32ToPPM(const char* dstfilename, unsigned int W, unsigned int H, Color32* data);
extern int LoadAndFormatPPM(const char* filename, unsigned int *W, unsigned int *H, Color32** data);

#define C565_5_MASK 0xF8
#define C565_6_MASK 0xFC

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

Uint32 ColorLuminance(const Color32 color)
{
	return 0.2126 * color.r + 0.7152 * color.g * 2 + 0.0722 * color.b;
}

Uint32 DistanceColor32(Color32 r1, Color32 r2)
{
	uint16_t red1, red2, green1, green2, blue1, blue2;
	red1 = r1.r;
	red2 = r2.r;
	green1 = r1.g;
	green2 = r2.g;
	blue1 = r1.b;
	blue2 = r2.b;

	return (red1 - red2) * (red1 - red2) + (green1 - green2) * (green1 - green2) + (blue1 - blue2) * (blue1 - blue2);
}

void GetMaxMinColors(Color32 *colors, Color32* minColor, Color32* maxColor)
{
	Uint32 maxLuminance = 0, minLuminance = 0xffffffff;
	for (int i = 0; i < 16; i++)
	{
		Uint32 luminance = ColorLuminance(colors[i]);
		if (luminance > maxLuminance)
		{
			maxLuminance = luminance;
			memcpy(maxColor, &colors[i], sizeof(Color32));
		}
		if (luminance < minLuminance)
		{
			minLuminance = luminance;
			memcpy(minColor, &colors[i], sizeof(Color32));
		}
	}
}

void CompressDXT1(Color32* colors, BlockDXT1* block)
{
	Color32 minColor, maxColor;
	GetMaxMinColors(colors, &minColor, &maxColor);
	minColor.a = 0, maxColor.a = 0;
	minColor.r = (minColor.r & C565_5_MASK) | (minColor.r >> 5);
	minColor.g = (minColor.g & C565_6_MASK) | (minColor.g >> 6);
	minColor.b = (minColor.b & C565_5_MASK) | (minColor.b >> 5);

	maxColor.r = (maxColor.r & C565_5_MASK) | (maxColor.r >> 5);
	maxColor.g = (maxColor.g & C565_6_MASK) | (maxColor.g >> 6);
	maxColor.b = (maxColor.b & C565_5_MASK) | (maxColor.b >> 5);

	Color32 linear[4];
	linear[0] = maxColor;
	linear[1] = minColor;
	linear[2].r = (2 * linear[0].r + linear[1].r) / 3;
	linear[2].g = (2 * linear[0].g + linear[1].g) / 3;
	linear[2].b = (2 * linear[0].b + linear[1].b) / 3;
	linear[3].r = (linear[0].r + 2 * linear[1].r) / 3;
	linear[3].g = (linear[0].g + 2 * linear[1].g) / 3;
	linear[3].b = (linear[0].b + 2 * linear[1].b) / 3;

	Uint8 indices[16];
	for (int i = 0; i < 16; i++)
	{
		Uint32 minDis = 0xffffffff;
		for (int j = 0; j < 4; j++)
		{
			Uint32 dis = DistanceColor32(colors[i], linear[j]);
			if (dis < minDis)
			{
				indices[i] = j;
			}
		}
	}
	block->col0.u = (linear[0].r << 11) | (linear[0].g << 5) | linear[0].b;
	block->col1.u = (linear[1].r << 11) | (linear[1].g << 5) | linear[1].b;

	Uint32 IntIndices = 0;
	for (int i = 0; i < 16; i++)
	{
		IntIndices |= (indices[i] << (2*i));
	}
	block->indices = IntIndices;
}

int LoadDXT1ToColor32(const char* ddsfile, Color32** img, uint16_t *W, uint16_t *H)
{
	FILE* fp = fopen(ddsfile, "rb");
	if (fp == NULL)
		return -1;
	DDSHeader *srcHeader = (DDSHeader*)malloc(sizeof(DDSHeader));
	fread(srcHeader, sizeof(DDSHeader), 1, fp);

	printf("Header file loaded.\n");
	printf("Header fourcc: %04x size: %d flags: %4x height: %d width: %d pitch: %d depth: %d mipmapcount: %d notused: %d",
		srcHeader->fourcc, srcHeader->size, srcHeader->flags, srcHeader->height, srcHeader->width, srcHeader->pitch,
		srcHeader->depth, srcHeader->mipmapcount, srcHeader->notused);
	printf("format size: %u fourcc: %04x flags %u bitcount %u rmask %u gmask %u bmask %u amsk %u",
		srcHeader->pf.size, srcHeader->pf.fourcc, srcHeader->pf.flags, srcHeader->pf.bitcount, srcHeader->pf.rmask, srcHeader->pf.gmask,
		srcHeader->pf.bmask, srcHeader->pf.amask);

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
		}
	}
	return 0;
}

int CompressImageData2DDS(const char* dst, const Uint32 width, const Uint32 height, Color32* img)
{
	// convert linear color to block color
	Color32 *block = (Color32*)malloc(width * height * 4);
	Uint32 blockWidth = (width + 3) / 4;
	Uint32 blockHeight = (height + 3) / 4;
	for (int blocky = 0; blocky < blockHeight; blocky++)
	{
		for (int blockx = 0; blockx < blockWidth; blockx++)
		{
			Uint32 blockoffset = blocky * blockWidth + blockx;
			Uint32 imgoffset = blocky * blockWidth * 16 + blockx * 4;
			for (int pixelIndex = 0; pixelIndex < 16; pixelIndex++)
			{
				int row = pixelIndex / 4;
				int col = pixelIndex % 4;
				block[blockoffset * 16 + pixelIndex] = img[imgoffset + row * width + col];
			}
		}
	}

	BlockDXT1* blockDxt1 = (BlockDXT1*)malloc(blockWidth * blockHeight * 8);
	for (int i = 0; i < blockWidth * blockHeight; i++)
	{
		CompressDXT1(block + i * 16, blockDxt1 + i);
	}

	SaveBlockDXT1ToDDS(dst, width, height, blockDxt1);

	return 0;
}

int SaveBlockDXT1ToDDS(const char* dst, const Uint32 width, const Uint32 height, BlockDXT1* data)
{
	FILE* fp = fopen(dst, "wb");
	if (fp == NULL)
	{
		return -1;
	}

	//Header fourcc: 20534444 size: 124 flags: 81007 height: 512 width: 512 pitch: 131072 depth: 0 mipmapcount: 0 notused: 0
	// Header fourcc: 20534444 size: 124 flags: 81007 height: 512 width: 512 pitch: 131072 depth: 0 mipmapcount: 0 notused: 0
	// format size: 32 fourcc: 31545844 flags x   4 bitcount 0 rmask 0 gmask 0 bmask 0 amsk 0

	DDSHeader header;
	DDSPixelFormat fmt;

	fmt.fourcc = MAKEFOURCC('D', 'X', 'T', '1');
	fmt.flags = 0x00000004U;
	fmt.size = sizeof(DDSPixelFormat);
	fmt.bitcount = 0;
	fmt.rmask = 0;
	fmt.gmask = 0;
	fmt.bmask = 0;
	fmt.amask = 0;

	header.fourcc = MAKEFOURCC('D', 'D', 'S', ' ');
	header.size = 124;
	header.flags = 0x81007;
	header.height = height;
	header.width = width;
	header.pitch = ((width + 3) / 4) * ((height + 3) / 4) * 8;
	header.depth = 0;
	header.mipmapcount = 0;
	memset(header.reserved, 0, sizeof(header.reserved));

	header.pf = fmt;
	header.caps.caps1 = 0x00001000U;
	header.caps.caps2 = 0;
	header.caps.caps3 = 0;
	header.caps.caps4 = 0;
	header.notused = 0;

	fwrite(&header, sizeof(DDSHeader), 1, fp);
	fwrite(data, sizeof(BlockDXT1), ((width + 3) / 4) * ((height + 3) / 4), fp);
	fclose(fp);
	return 0;
}

// unit test
void DXT1LoadTest(const char* filename, const char* dstfile)
{
	Color32 *img = NULL;
	uint16_t w, h;

	if(LoadDXT1ToColor32(filename, &img, &w, &h) !=0)
		return;

	WriteColor32ToPPM(dstfile, w, h, img);
}

void copyDxt1(const char* src, const char* dst)
{
	FILE* fp = fopen(src, "rb");
	if (fp == NULL)
		return;
	DDSHeader *srcHeader = (DDSHeader*)malloc(sizeof(DDSHeader));
	fread(srcHeader, sizeof(DDSHeader), 1, fp);
	Uint32 w, h;
	w = srcHeader->width;
	h = srcHeader->height;

	Uint32 blockSize = ((w + 3) / 4) * ((h + 3) / 4);
	BlockDXT1* blockdata = (BlockDXT1*)malloc(blockSize * 8); // 4096 * 4096 oversize?
	fread(blockdata, sizeof(BlockDXT1), blockSize, fp);
	fclose(fp);

	SaveBlockDXT1ToDDS(dst, w, h, blockdata);
}

void compressPPM(const char*src, const char* dst)
{
	Uint32 w, h;
	Color32 *data = NULL;
	if(LoadAndFormatPPM(src, &w, &h, &data) !=0)
		return;
	printf("load ppm done\n");

	CompressImageData2DDS(dst, w, h, data);
}


