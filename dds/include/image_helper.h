#pragma once

#include <stdint.h>

typedef uint8_t Uint8;
typedef uint32_t Uint32;

/*
Reference SDL_PIXEL_FORMAT.
Ignore Rloss, Gloss, Gloss
*/
struct ImageFormat
{
	Uint8 BitsPerPixel;
	Uint8 BytesPerPixel;

	Uint8 Alpha;  // 0, 1
	Uint8 padding;

	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;

	Uint8 Rshift;
	Uint8 Gshift;
	Uint8 Bshift;
	Uint8 Ashift;
};

constexpr ImageFormat IMGFMT_PPMP6_255 =
{ 24, 3, 0, 0, 0xff0000, 0xff00, 0xff, 0, 16, 8, 0, 0 };


union Color32
{
	struct {
		Uint8 b;
		Uint8 g;
		Uint8 r;
		Uint8 a;
	};
	Uint32 u;
};