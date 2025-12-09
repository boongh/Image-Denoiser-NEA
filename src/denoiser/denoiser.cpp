#pragma once

#include "denoiser.h"
#include <unordered_map>
#include <cmath>
#include <Eigen/Dense>
#include <iostream>
#include <memory>

#define doBackward 1
#define doDivide 0

int Denoiser::SmoothLF(std::span<PixelRGBA> src, unsigned int width, unsigned int height, int halfWidth, int halfHeight, double strn)
{

	std::vector<PixelRGBA> dst = std::vector<PixelRGBA>(src.begin(), src.end());
	int xPos = 0;
	int yPos = 0;

	unsigned int postemp = 0;
	unsigned int Accum = 0;

	PixelRGBA pinit;
	PixelRGBA pavg;

	unsigned int sumR, sumG, sumB;

	try {

		for (int pixelpos = 0; pixelpos < src.size(); ++pixelpos) {
			pavg.Clear();
			PosDecompose(pixelpos, width, height, &xPos, &yPos);

			Accum = 0;
			sumR = sumG = sumB = 0;

			
#if 0
			for (unsigned int y = std::max(yPos - (int)halfHeight, 0); y < std::min(yPos + (int)halfHeight, (int)height); y++) {
				for (unsigned int x = std::max(xPos - (int)halfWidth, 0); x < std::min(xPos + (int)halfWidth, (int)width); x++) {
					postemp = PosCompose(x, y, width);
					PixelRGBA srcpixel = src[postemp];
					sumR += srcpixel.r;
					sumG += srcpixel.g;
					sumB += srcpixel.b;
					Accum++;
				}
			}
#else

			for (int y = -(int)halfHeight; y <= halfHeight; y++) {
				for (int x = -(int)halfWidth; x <= halfWidth; x++) {
					int actX = x + xPos;
					int actY = y + yPos;

					// prevents invalid location
					if (actX < 0 || actX >= width || actY < 0 || actY >= height || (y == 0 && x == 0)) continue;
					postemp = PosCompose(x + xPos, y + yPos, width);
					PixelRGBA srcpixel = src[postemp];
					sumR += srcpixel.r;
					sumG += srcpixel.g;
					sumB += srcpixel.b;
					Accum++;
				}
			}
#endif // 0


#ifdef DEBUG	
			if (Accum == 0) {
				printf("Latest pixel %d before crash (%d, %d, %d) with count %d", pixelpos, sumR, sumG, sumB, Accum);
				return -1;
			}
#endif // DEBUG

			pinit = src[pixelpos];
			sumR /= Accum; sumG /= Accum;  sumB /= Accum;

			dst[pixelpos] = PixelRGBA(
				lerp<uint8_t>(pinit.r, sumR, strn),
				lerp<uint8_t>(pinit.g, sumG, strn),
				lerp<uint8_t>(pinit.b, sumB, strn),
				pinit.a
			);

		}

		memcpy(src.data(), dst.data(), width * height * sizeof(PixelRGBA));
		return 0;
	}
	catch (std::exception e) {
		return -1;
	}
}

int Denoiser::BilateralFilter(std::span<const PixelRGBA> src, unsigned int width, unsigned int height, int halfWidth, int halfHeight, double strnSpatial, double strnIntensity) {
	auto denoisedimage = std::vector<PixelRGBA>(src.size());

	double inverseSpatial = 1.0 / strnSpatial;
	double inverseIntensity = 1.0 / strnIntensity;


	//Cache construction for distance attenuation
	std::vector<float> cacheDistAtten((2 * halfWidth + 1) * (2 * halfHeight + 1));
	int index = 0;

	for (int x = -halfWidth; x <= halfWidth; x++) {
		for (int y = -halfHeight; y <= halfHeight; y++) {
			double xNorm = x * inverseSpatial;
			double yNorm = y * inverseSpatial;
			int dist = xNorm * xNorm + yNorm * yNorm;
			cacheDistAtten[index++] = Bilateral::RangeAttenuation(dist, inverseSpatial);
		}
	}

	index = 0;
	constexpr double INVERSECOLOR = 1 / 255.0;
	double INVERSEWIDTH = 1.0 / width;
	double INVERSEHEIGHT = 1.0 / height;
	int loopCount = 0;

	for (int pixelnum = 0; pixelnum < src.size(); pixelnum++) {
		int xPos = pixelnum % width;
		int yPos = pixelnum / width;

		double sumR = 0;
		double sumG = 0;
		double sumB = 0;

		double sumRw = 0;
		double sumGw = 0;
		double sumBw = 0;

		double sumW = 0;

		PixelRGBA pinit = src[pixelnum];

		//MathVector<float, 3> pinitVecRGB(
		//	(float)pinit.r * INVERSECOLOR * inverseIntensity,
		//	(float)pinit.g * INVERSECOLOR * inverseIntensity,
		//	(float)pinit.b * INVERSECOLOR * inverseIntensity
		//);

		index = 0;

		// Decompose pixel position

		for (int y = -halfHeight; y <= halfHeight; y++) {
			int actY = y + yPos;
			if (actY < 0 || actY >= height) {
				index += 2 * halfWidth + 1;
				continue;
			};

			for (int x = -halfWidth; x <= halfWidth; x++) {
				int actX = x + xPos;
				if (actX < 0 || actX >= width) {
					index++;
					continue;
				};

				// prevents invalid location

				unsigned int postemp = actX + actY* width;
				PixelRGBA srcpixel = src[postemp];
#if 1
				{

					//Normalize the distance

					double xd = (double)x / width;
					double yd = (double)y / height;

					double rDiffNorm = ((double)pinit.r - srcpixel.r) * INVERSECOLOR;
					double gDiffNorm = ((double)pinit.g - srcpixel.g) * INVERSECOLOR;
					double bDiffNorm = ((double)pinit.b - srcpixel.b) * INVERSECOLOR;

					// Calculate the weight for each channel
					// Normalize intensity
					double rw = Bilateral::IntensityAttenuation(rDiffNorm * rDiffNorm, inverseIntensity) * cacheDistAtten[index];
					double gw = Bilateral::IntensityAttenuation(gDiffNorm * gDiffNorm, inverseIntensity) * cacheDistAtten[index];
					double bw = Bilateral::IntensityAttenuation(bDiffNorm * bDiffNorm, inverseIntensity) * cacheDistAtten[index];

					index++;

					sumR += srcpixel.r * rw;
					sumG += srcpixel.g * gw;
					sumB += srcpixel.b * bw;

					sumRw += rw;
					sumGw += gw;
					sumBw += bw;
				}
#else
				{

					MathVector<float, 3> srcVecRGB(
						(float)srcpixel.r * INVERSECOLOR * inverseIntensity,
						(float)srcpixel.g * INVERSECOLOR * inverseIntensity,
						(float)srcpixel.b * INVERSECOLOR * inverseIntensity
					);

					double omega = Bilateral::VectorAttenuation(pinitVecRGB, srcVecRGB, 1) * cacheDistAtten[index++];

					sumR += srcpixel.r * omega;
					sumG += srcpixel.g * omega;
					sumB += srcpixel.b * omega;

					sumW += omega;


				}
#endif
					loopCount++;
			}

		}
#ifdef DEBUG

		if (loopCount % 100000 == 0) std::println("{}th loop \n", loopCount);
#endif // DEBUG

		denoisedimage[pixelnum].r = static_cast<uint8_t>(sumR / sumRw);
		denoisedimage[pixelnum].g = static_cast<uint8_t>(sumG / sumGw);
		denoisedimage[pixelnum].b = static_cast<uint8_t>(sumB / sumBw);
	}


	memcpy((void*)src.data(), (void*)denoisedimage.data(), sizeof(PixelRGBA)* src.size());
	return 0;
}

int Denoiser::DWT(std::span<PixelRGBA> src, unsigned int width, unsigned int height, int decimationLevel) {
	Denoiser::DWT::DecTree dectree = Denoiser::DWT::DecTree(src, width, height);

	dectree.ExpandTree(decimationLevel);

	//Initialize thresholder
	Denoiser::VisuShrink visu = Denoiser::VisuShrink();
	dectree.Thresholding(visu);
	dectree.CollapseTree();

	auto resultImage = dectree.GetImageRGB();


	//Copies result into src (inplace)
	memcpy((void*)src.data(), (void*)resultImage.data.data(), sizeof(PixelRGBA) * src.size());

	

	return 0;
}

inline double Denoiser::Bilateral::RangeAttenuation(double distanceSquared, double inverseSD)
{
	return exp(-(distanceSquared) * (0.5 * inverseSD * inverseSD));
}

inline double Denoiser::Bilateral::IntensityAttenuation(double distanceSquared, double inverseSD)
{
	return exp(-(distanceSquared) * (0.5 * inverseSD * inverseSD));
}