#pragma once

#include <span>
#include <algorithm>
#include <FileFormats.h>


class Denoiser {
public:
	/// <summary>
	/// 
	/// </summary>
	/// <param name="src"></param>
	/// <param name="dst"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <param name="strn"></param>
	static int SmoothLF(
		std::span<PixelRGBA> src,
		std::span<PixelRGBA> dst,
		unsigned int width,
		unsigned int height,
		unsigned int strn
	) {

		if (src.size() != dst.size()) return -1;

		unsigned int xPos = 0;
		unsigned int yPos = 0;

		PixelRGBA pavg;
		for (int pixelpos = 0; pixelpos < src.size();  ++pixelpos) {
			PosDecompose(pixelpos, width, height, &xPos, &yPos);
			pavg
			for (int x = std::min(xPos, (unsigned int)0); x < std::max(xPos + 2, (unsigned int)width - 1); x++) {
				for (int y = std::min(yPos, (unsigned int)0); y < std::max(yPos + 2, (unsigned int)height - 1); y++) {
				}
			}
		}
	}
private:
	static void PosDecompose(
		unsigned int pos,
		unsigned int width,
		unsigned int height,
		unsigned int* xstr,
		unsigned int* ystr) {

		*xstr = pos % width;
		*ystr = pos / height;
	}
};