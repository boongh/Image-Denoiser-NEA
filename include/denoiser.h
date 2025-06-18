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
	static std::vector<PixelRGBA> SmoothLF(std::span<const PixelRGBA> src,
		unsigned int width, unsigned int height, 
		int halfWidth, int halfHeight, 
		double strn);

	static void PosDecompose(
		unsigned int pos,
		unsigned int width,
		unsigned int height,
		int* xstr,
		int* ystr);

	static inline int PosCompose(unsigned int xstr,
		unsigned int ystr,
		unsigned int width);

	template <typename T>
	static inline T lerp(T a, T b, double t) {
		return static_cast<T>((1 - t) * a + b * t);
	}
};