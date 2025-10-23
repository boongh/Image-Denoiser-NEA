#include "denoiser.h"
#include <unordered_map>
#include <cmath>
#include <Eigen/Dense>
#include <iostream>
#include <memory>

#define doBackward 1
#define doDivide 0

std::vector<PixelRGBA> Denoiser::SmoothLF(std::span<const PixelRGBA> src, unsigned int width, unsigned int height, int halfWidth, int halfHeight, double strn)
{
	std::vector<PixelRGBA> dst = std::vector<PixelRGBA>(src.size());
	int xPos = 0;
	int yPos = 0;

	unsigned int postemp = 0;
	unsigned int cum = 0;

	PixelRGBA pinit;
	PixelRGBA pavg;

	unsigned int sumR, sumG, sumB;

	try {

		for (int pixelpos = 0; pixelpos < src.size(); ++pixelpos) {
			pavg.Clear();
			PosDecompose(pixelpos, width, height, &xPos, &yPos);

			cum = 0;
			sumR = sumG = sumB = 0;

			
#if 0
			for (unsigned int y = std::max(yPos - (int)halfHeight, 0); y < std::min(yPos + (int)halfHeight, (int)height); y++) {
				for (unsigned int x = std::max(xPos - (int)halfWidth, 0); x < std::min(xPos + (int)halfWidth, (int)width); x++) {
					postemp = PosCompose(x, y, width);
					PixelRGBA srcpixel = src[postemp];
					sumR += srcpixel.r;
					sumG += srcpixel.g;
					sumB += srcpixel.b;
					cum++;
				}
			}
#else

			for (int y = - (int)halfHeight; y <= halfHeight; y++) {
				for (int x = - (int)halfWidth; x <= halfWidth; x++) {
					int actX = x + xPos;
					int actY = y + yPos;

					// prevents invalid location
					if (actX < 0 || actX >= width || actY < 0 || actY >= height || (y == 0 && x == 0)) continue;
					postemp = PosCompose(x + xPos, y + yPos, width);
					PixelRGBA srcpixel = src[postemp];
					sumR += srcpixel.r;
					sumG += srcpixel.g;
					sumB += srcpixel.b;
					cum++;
				}
			}
#endif // 0


#ifdef DEBUG	
			if (cum == 0) {
				printf("Latest pixel %d before crash (%d, %d, %d) with count %d", pixelpos, sumR, sumG, sumB, cum);
				return std::vector<PixelRGBA>(0);
			}
#endif // DEBUG

			pinit = src[pixelpos];
			sumR /= cum; sumG /= cum;  sumB /= cum;

			dst[pixelpos] = PixelRGBA(
				lerp<uint8_t>(pinit.r, sumR, strn),
				lerp<uint8_t>(pinit.g, sumG, strn),
				lerp<uint8_t>(pinit.b, sumB, strn),
				pinit.a
			);

		}
		return dst;
	}
	catch (std::exception e) {
		return dst;
	}
}

std::vector<PixelRGBA> Denoiser::BilateralFilter(std::span<const PixelRGBA> src, unsigned int width, unsigned int height, int halfWidth, int halfHeight, double strnSpatial, double strnIntensity) {
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

	return denoisedimage;
}

void Denoiser::PosDecompose(
	unsigned int pos, 
	unsigned int width, 
	unsigned int height, 
	int* xstr, 
	int* ystr) {
	*xstr = pos % width;
	*ystr = pos / width;
}

inline int Denoiser::PosCompose(unsigned int xstr,
	unsigned int ystr,
	unsigned int width) {
	return xstr + ystr * width;
}

std::array<float, 3> Denoiser::LRGBtoYCbCr(float R, float G, float B)
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


std::array<float, 3> Denoiser::YCbCrtoLRGB(float Y, float Cb, float Cr)
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

float Denoiser::Channel_sRGBtoLRGB(float V)
{
	return (V <= 0.04045) ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4);
}

float Denoiser::Channel_LRGBtosRGB(float V)
{
	return (V <= 0.0031308) ? 12.92 * V : 1.055 * std::pow(V, 1 / 2.4) - 0.055;
}

std::array<float, 3> Denoiser::sRGBtoLRGB(float R, float G, float B)
{
	return std::array<float, 3>{
		Channel_sRGBtoLRGB(R),
		Channel_sRGBtoLRGB(G),
		Channel_sRGBtoLRGB(B)
	};
}

std::array<float, 3> Denoiser::LRGBtosRGB(float R, float G, float B)
{
	return std::array<float, 3>{
		Channel_LRGBtosRGB(R),
		Channel_LRGBtosRGB(G),
		Channel_LRGBtosRGB(B)
	};
}

inline double Denoiser::Bilateral::RangeAttenuation(double distanceSquared, double inverseSD)
{
	return exp(-(distanceSquared) * (0.5 * inverseSD * inverseSD));
}

inline double Denoiser::Bilateral::IntensityAttenuation(double distanceSquared, double inverseSD)
{
	return exp(-(distanceSquared) * (0.5 * inverseSD * inverseSD));
}