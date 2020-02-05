#pragma once
#include "image_helper.h"

#if !defined(MAKEFOURCC)
#    define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ( (unsigned int)(ch0)        | ((unsigned int)(ch1) << 8) | \
      ( (unsigned int)(ch2) << 16) | ((unsigned int)(ch3) << 24) )
#endif

struct DDSPixelFormat
{
	Uint32 size;
	Uint32 flags;
	Uint32 fourcc;
	Uint32 bitcount;
	Uint32 rmask;
	Uint32 gmask;
	Uint32 bmask;
	Uint32 amask;
};

struct DDSCaps
{
	Uint32 caps1;
	Uint32 caps2;
	Uint32 caps3;
	Uint32 caps4;
};

struct DDSHeader
{
	Uint32 fourcc;
	Uint32 size;
	Uint32 flags;
	Uint32 height;
	Uint32 width;
	Uint32 pitch;
	Uint32 depth;
	Uint32 mipmapcount;
	Uint32 reserved[11];
	DDSPixelFormat pf;
	DDSCaps caps;
	Uint32 notused;
};

union Color16
{
	struct
	{
		uint16_t b : 5;
		uint16_t g : 6;
		uint16_t r : 5;
	};
	uint16_t u;
};

struct BlockDXT1
{
	Color16 col0;
	Color16 col1;
	union
	{
		unsigned char row[4];
		unsigned int indices;
	};

	void decompress(Color32 color[]) const;
};

void CompressDXT1(Color32[16], BlockDXT1*);

//int CompressPPM2DDS(const char* ppmfile, const char* ddsfile);
//int DecompressDDS2PPM(const char* ddsfile, const char* ppmfile);
int CompressImageData2DDS(const char* dst, const Uint32 width, const Uint32 height, Color32* img);

int SaveBlockDXT1ToDDS(const char* dst, const Uint32 width, const Uint32 height, BlockDXT1* data);

int LoadDXT1ToColor32(const char* ddsfile, Color32** img, uint16_t *W, uint16_t *H);

// unit test
void DXT1LoadTest(const char* filename, const char* dstfile);
void copyDxt1(const char* src, const char* dst);
void compressPPM(const char*src, const char* dst);