#pragma once

#include "mathsutils.h"
#include <algorithm>
#include <cmath>

inline void PosDecompose(
	unsigned int pos,
	unsigned int width,
	unsigned int height,
	int* xstr,
	int* ystr) {
	*xstr = pos % width;
	*ystr = pos / width;
}

inline int PosCompose(unsigned int xstr,
	unsigned int ystr,
	unsigned int width) {
	return xstr + ystr * width;
}

std::array<float, 3> LRGBtoYCbCr(float R, float G, float B)
{
	// Y' (Luminance)
	float Y = 0.2126f * R + 0.7152f * G + 0.0722f * B;

	// Cb (Blue-difference chroma)
	// Cb = 0.5 * (B - Y) / (1 - K_b) where K_b = 0.0722
	float Cb = 0.5f * (B - Y) / (1.0f - 0.0722f);

	// Cr (Red-difference chroma)
	// Cr = 0.5 * (R - Y) / (1 - K_r) where K_r = 0.2126
	float Cr = 0.5f * (R - Y) / (1.0f - 0.2126f);


	// The YCbCr values are returned in the order Y, Cb, Cr
	return std::array<float, 3>{Y, Cb, Cr};
}


std::array<float, 3> YCbCrtoLRGB(float Y, float Cb, float Cr)
{
	// The coefficients are the inverse matrix of the RGB to YCbCr conversion.
	float R = Y + 1.5748f * Cr;
	float G = Y - 0.1873f * Cb - 0.4681f * Cr;
	float B = Y + 1.8556f * Cb;

	// Return the RGB values, clamping them to the [0.0, 1.0] range
	// to handle potential over/underflow from floating-point arithmetic.
	return std::array<float, 3>{
		std::clamp(R, 0.0f, 1.0f),
			std::clamp(G, 0.0f, 1.0f),
			std::clamp(B, 0.0f, 1.0f)};
}

float Channel_sRGBtoLRGB(float V)
{
	return (V <= 0.04045) ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4);
}

float Channel_LRGBtosRGB(float V)
{
	return (V <= 0.0031308) ? 12.92 * V : 1.055 * std::pow(V, 1 / 2.4) - 0.055;
}

std::array<float, 3> sRGBtoLRGB(float R, float G, float B)
{
	return std::array<float, 3>{
		Channel_sRGBtoLRGB(R),
			Channel_sRGBtoLRGB(G),
			Channel_sRGBtoLRGB(B)
	};
}

std::array<float, 3> LRGBtosRGB(float R, float G, float B)
{
	return std::array<float, 3>{
		Channel_LRGBtosRGB(R),
			Channel_LRGBtosRGB(G),
			Channel_LRGBtosRGB(B)
	};
}