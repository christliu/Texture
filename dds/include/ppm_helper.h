#pragma once

#include "image_helper.h"

int LoadPPM(const char* filename, unsigned int* W, unsigned int* H, unsigned int* channels, uint8_t** data);
int LoadAndFormatPPM(const char* filename, unsigned int *W, unsigned int *H, Color32** data);
int WriteColor32ToPPM(const char* dstfilename, unsigned int W, unsigned int H, Color32* data);

// unit test
int LoadPPMTest(const char* filename, const char* dstfile);
