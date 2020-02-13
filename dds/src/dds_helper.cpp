#include "dds_helper.h"
#include "ppm_helper.h"
#include <malloc.h>
#include <stdio.h>
#include <cstring>
#include <cfloat>

extern int WriteColor32ToPPM(const char* dstfilename, unsigned int W, unsigned int H, Color32* data);
extern int LoadAndFormatPPM(const char* filename, unsigned int *W, unsigned int *H, Color32** data);

#define C565_5_MASK 0xF8
#define C565_6_MASK 0xFC

struct Pixel128
{
	float r, g, b, a;
};

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


void GetMaxMinColorsOfBoundingBox(Pixel128 pixels[16], Pixel128* minColor, Pixel128* maxColor)
{
	Pixel128 maxPixel, minPixel;
	maxPixel = { 0.0f, 0.0f, 0.0f, 1.0f };
	minPixel = { 1.0f, 1.0f, 1.0f, 1.0f };
	for (int i = 0; i < 16; i++)
	{
		if (pixels[i].r > maxPixel.r)
			maxPixel.r = pixels[i].r;
		if (pixels[i].g > maxPixel.g)
			maxPixel.g = pixels[i].g;
		if (pixels[i].b > maxPixel.b)
			maxPixel.b = pixels[i].b;

		if (pixels[i].r < minPixel.r)
			minPixel.r = pixels[i].r;
		if (pixels[i].b < minPixel.b)
			minPixel.b = pixels[i].b;
		if (pixels[i].g < minPixel.b)
			minPixel.g = pixels[i].g;
	}
	Pixel128 minMax = { maxPixel.r - minPixel.r, maxPixel.g - minPixel.g, maxPixel.b - minPixel.b, 1.0 };
	float fMinMax = minMax.r * minMax.r + minMax.g * minMax.g + minMax.b * minMax.b;
	if (fMinMax < FLT_MIN)
	{
		minColor->r = minPixel.r; minColor->g = minPixel.g; minColor->b = minPixel.b;
		maxColor->r = maxPixel.r; maxColor->g = maxPixel.g; maxColor->b = maxPixel.b;
		return;
	}
	float fMinMaxInv = 1 / fMinMax;
	Pixel128 Dir = { minMax.r * fMinMaxInv, minMax.g * fMinMaxInv, minMax.b * fMinMaxInv, 0.0f};
	Pixel128 Mid = { (minPixel.r + maxPixel.r) * 0.5, (minPixel.g + maxPixel.g) * 0.5, (minPixel.b + maxPixel.b) * 0.5, 0.0f };
	float fDir[4];
	for (int i = 0; i < 16; i++)
	{
		Pixel128 pixel;
		pixel.r = (pixels[i].r - Mid.r) * Dir.r;
		pixel.g = (pixels[i].g - Mid.g) * Dir.g;
		pixel.b = (pixels[i].b - Mid.b) * Dir.b;
		pixel.a = 0.0f;

		fDir[0] += (pixel.r + pixel.g + pixel.b);
		fDir[1] += (pixel.r + pixel.g - pixel.b);
		fDir[2] += (pixel.r - pixel.g + pixel.b);
		fDir[3] += (pixel.r - pixel.g - pixel.b);
	}
	float fMax = fDir[0];
	int index = 0;
	for (int i = 0; i < 4; i++)
	{
		if (fDir[i] > fMax)
		{
			index = i;
			fMax = fDir[i];
		}
	}
	if (index & 2)
	{
		float f = minPixel.g;
		minPixel.g = maxPixel.g;
		maxPixel.g = f;
	}
	if (index & 1)
	{
		float f = minPixel.b;
		minPixel.b = maxPixel.b;
		maxPixel.b = f;
	}
	minColor->r = minPixel.r; minColor->g = minPixel.g; minColor->b = minPixel.b;
	maxColor->r = maxPixel.r; maxColor->g = maxPixel.g; maxColor->b = maxPixel.b;
}

void CompressDXT1HDR(Color32* colors, BlockDXT1* block)
{
	Pixel128 pixels[16];
	for (int i = 0; i < 16; i++)
	{
		Pixel128 p;
		p.r = colors[i].r / 255.0f;
		p.g = colors[i].g / 255.0f;
		p.b = colors[i].b / 255.0f;
		p.a = 1.0f;
		pixels[i] = p;
	}
	Pixel128 color0, color1;
	GetMaxMinColorsOfBoundingBox(pixels, &color0, &color1);
	Pixel128 steps[4];
	steps[0].r = color1.r; steps[0].g = color1.g; steps[0].b = color1.b; steps[0].a = 0.0f;
	steps[1].r = color0.r; steps[1].g = color0.g; steps[1].b = color0.b; steps[1].a = 0.0f;
	steps[2].r = (2 * color1.r + color0.r) / 3; steps[2].g = (2 * color1.g + color0.g) / 3;
	steps[2].b = (2 * color1.b + color0.b) / 3; steps[2].a = 1.0f;
	steps[3].r = (color1.r + 2 * color0.r) / 3; steps[3].g = (color1.g + 2 * color0.g) / 3;
	steps[3].b = (color1.b + 2 * color0.b) / 3; steps[3].r = 1.0f;

	int indices[16];
	for (int i = 0; i < 16; i++)
	{
		int index = 0;
		float dis = (pixels[i].r - steps[0].r) * (pixels[i].r - steps[0].r) +
			(pixels[i].g - steps[0].g) * (pixels[i].g - steps[0].g) +
			(pixels[i].b - steps[0].b) * (pixels[i].b - steps[0].b);
		for (int s = 1; s < 4; s++)
		{
			float tmp = (pixels[i].r - steps[s].r) * (pixels[i].r - steps[s].r) +
				(pixels[i].g - steps[s].g) * (pixels[i].g - steps[s].g) +
				(pixels[i].b - steps[s].b) * (pixels[i].b - steps[s].b);
			if (tmp < dis)
			{
				index = s;
				dis = tmp;
			}
		}
		indices[i] = index;
	}
	block->col0.u = static_cast<uint16_t>(
		(static_cast<int32_t>(color1.r * 31.0f + 0.5f) << 11)
		| (static_cast<int32_t>(color1.g * 63.0f + 0.5f) << 5)
		| (static_cast<int32_t>(color1.b * 31.0f + 0.5f) << 0));

	block->col1.u = static_cast<uint16_t>(
		(static_cast<int32_t>(color0.r * 31.0f + 0.5f) << 11)
		| (static_cast<int32_t>(color0.g * 63.0f + 0.5f) << 5)
		| (static_cast<int32_t>(color0.b * 31.0f + 0.5f) << 0));


	Uint32 IntIndices = 0;
	for (int i = 0; i < 16; i++)
	{
		IntIndices = (indices[i] << 30) | (IntIndices >> 2);
		//IntIndices |= (indices[i] << (2 * i));
	}
	block->indices = IntIndices;
}


/*
1. find bounding box. 
2. find mid of box. and 4 direction of the axis.
3. eval which axis direction suits most(dot of every point is lowest).
4. 
*/
void CompressDXT1(Color32* colors, BlockDXT1* block)
{
	Color32 minColor, maxColor;
	GetMaxMinColors(colors, &minColor, &maxColor);
	minColor.a = 0, maxColor.a = 0;
	/*minColor.r = (minColor.r & C565_5_MASK) | (minColor.r >> 5);
	minColor.g = (minColor.g & C565_6_MASK) | (minColor.g >> 6);
	minColor.b = (minColor.b & C565_5_MASK) | (minColor.b >> 5);

	maxColor.r = (maxColor.r & C565_5_MASK) | (maxColor.r >> 5);
	maxColor.g = (maxColor.g & C565_6_MASK) | (maxColor.g >> 6);
	maxColor.b = (maxColor.b & C565_5_MASK) | (maxColor.b >> 5);*/

	Uint32 r, g, b;

	Color32 linear[4];
	linear[0] = maxColor;
	linear[1] = minColor;

	r = (2 * (Uint32)linear[0].r + (Uint32)linear[1].r) / 3;
	g = (2 * (Uint32)linear[0].g + (Uint32)linear[1].g) / 3;
	b = (2 * (Uint32)linear[0].b + (Uint32)linear[1].b) / 3;
	linear[2].r = r;
	linear[2].g = g;
	linear[2].b = b;

	r = ((Uint32)linear[0].r + 2 * (Uint32)linear[1].r) / 3;
	g = ((Uint32)linear[0].g + 2 * (Uint32)linear[1].g) / 3;
	b = ((Uint32)linear[0].b + 2 * (Uint32)linear[1].b) / 3;
	linear[3].r = r;
	linear[3].g = g;
	linear[3].b = b;

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
				minDis = dis;
			}
		}
	}

	//r = (Uint32)linear[0].r / 255.0 * 0x1f;
	//g = (Uint32)linear[0].g / 255.0 * 0x3f;
	//b = (Uint32)linear[0].b / 255.0 * 0x1f;
	//linear[0].r = r;
	//linear[0].g = g;
	//linear[0].b = b;

	//r = (Uint32)linear[1].r / 255.0 * 0x1f;
	//g = (Uint32)linear[1].g / 255.0 * 0x3f;
	//b = (Uint32)linear[1].b / 255.0 * 0x1f;

	//linear[1].r = r;
	//linear[1].g = g;
	//linear[1].b = b;

	//block->col0.u = (linear[0].r << 11) | (linear[0].g << 5) | linear[0].b;
	//block->col1.u = (linear[1].r << 11) | (linear[1].g << 5) | linear[1].b;

	block->col0.u = ((linear[0].r & 0xf8) << 8) | ((linear[0].g & 0xfc) << 3) | ((linear[0].b & 0xf8) >> 3);
	block->col1.u = ((linear[1].r & 0xf8) << 8) | ((linear[1].g & 0xfc) << 3) | ((linear[1].b & 0xf8) >> 3);

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

	int blockoffset = 0;

	//for (int i = 0; i < 16; i++)
	//{
	//	int off = blockoffset * 16 + i;
	//	printf("color %d r %u g %u b %u\n", off, block[off].r, block[off].g, block[off].b);
	//}

	BlockDXT1* blockDxt1 = (BlockDXT1*)malloc(blockWidth * blockHeight * 8);
	for (int i = 0; i < blockWidth * blockHeight; i++)
	{
		//CompressDXT1(block + i * 16, blockDxt1 + i);
		CompressDXT1HDR(block + i * 16, blockDxt1 + i);
	}

	BlockDXT1* blockDebug = blockDxt1 + blockoffset;
	printf("block %d col0 %u col1 %u indices %u\n", blockoffset, blockDebug->col0.u, blockDebug->col1.u, blockDebug->indices);

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


void readBlockDX1(const char* filename, int *length, BlockDXT1** data)
{
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL)
		return;
	DDSHeader *srcHeader = (DDSHeader*)malloc(sizeof(DDSHeader));
	fread(srcHeader, sizeof(DDSHeader), 1, fp);
	Uint32 w, h;
	w = srcHeader->width;
	h = srcHeader->height;

	*length = ((w + 3) / 4) * ((h + 3) / 4);
	*data = (BlockDXT1*)malloc(*length * 8);
	fread(*data, sizeof(BlockDXT1), *length, fp);
	fclose(fp);
}

void computeError(const char* dds1, const char* dds2)
{
	int len1, len2;
	BlockDXT1* block1 = NULL, *block2 = NULL;
	readBlockDX1(dds1, &len1, &block1);
	readBlockDX1(dds2, &len2, &block2);
	if (len1 != len2)
	{
		printf("size not equal !");
		return;
	}
	for (int offset = 0; offset < len1; offset++)
	{
		Color32 color1[16], color2[16];
		(block1 + offset)->decompress(color1);
		(block2 + offset)->decompress(color2);

		Uint32 error = 0;

		for (int i = 0; i < 16; i++)
		{
			int32_t r = color1[i].r > color2[i].r ? (color1[i].r - color2[i].r) : color2[i].r - color1[i].r;
			int32_t g = color1[i].g > color2[i].g ? (color1[i].g - color2[i].g) : color2[i].g - color1[i].g;
			int32_t b = color1[i].b > color2[i].b ? (color1[i].b - color2[i].b) : color2[i].b - color1[i].b;

			error = r * r + g * g + b * b;
		}
		if (error > 10000)
		{
			printf("error %u\n", error);
			printf("offset %u\n", offset);
		}
	}
}


