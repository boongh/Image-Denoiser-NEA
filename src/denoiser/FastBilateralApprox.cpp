#include "Denoiser.h"
#include <vector>
#include <algorithm>


static constexpr double INVERSECOLOR = 1.0 / 255.0;
constexpr double PI = 3.14159265358979323846;

class FastBilateralApproximation {



	static constexpr double INVERSECOLOR = 1.0 / 255.0;
	
	static double MaxDifference(std::span<const PixelRGBA> src,
		unsigned int width, unsigned int height,
		int halfWidth, int halfHeight) {
		double maxDiff = 0.0;
		for(int i = 0; i < src.size(); i++) {
			int xPos = i % width;
			int yPos = i / width;
			for (int y = -halfHeight; y <= halfHeight; y++) {
				int actY = y + yPos;
				if (actY < 0 || actY >= height) continue;
				for (int x = -halfWidth; x <= halfWidth; x++) {
					int actX = x + xPos;
					if (actX < 0 || actX >= width) continue;
					unsigned int postemp = actX + actY * width;
					PixelRGBA srcpixel = src[postemp];
					double rDiffNorm = std::abs(srcpixel.r * INVERSECOLOR - src[i].r * INVERSECOLOR);
					double gDiffNorm = std::abs(srcpixel.g * INVERSECOLOR - src[i].g * INVERSECOLOR);
					double bDiffNorm = std::abs(srcpixel.b * INVERSECOLOR - src[i].b * INVERSECOLOR);
					double diff = std::max({ rDiffNorm, gDiffNorm, bDiffNorm });
					maxDiff = std::max(maxDiff, diff);
				}
			}
		}
	}

	

};

static std::vector<double> GaussianOnRange(double min, double max, double sigma, double mean, double amplitude, double interval) {
	if (min >= max || sigma <= 0) {
		throw std::invalid_argument("Invalid range or sigma for GaussianOnRange.");
	}

	std::vector<double> result(std::abs(min - max) + 1 / interval);

	int index = 0;
	double inversesigmasquared = 1.0 / sigma / sigma;
	for (double v = min; v <= max; v += interval) {
		result[index++] = amplitude * std::exp(-0.5 * (v - mean) * (v - mean) * inversesigmasquared);
	}

	return result;
}

std::vector<PixelRGBA> Denoiser::FastBFApprox2(std::span<const PixelRGBA> src, unsigned int width, unsigned int height, int halfWidth, int halfHeight, double strnSpatial, double strnIntensity)
{
	return std::vector<PixelRGBA>();
}

void Denoiser::GaussianBlur(std::span<const PixelRGBA> src, std::span<PixelRGBA>& dst, unsigned int width, unsigned int height, int halfWidth, int halfHeight, double strn) {
#if 1
	//Naive gaussian blur implementation
	if (src.empty() || dst.empty() || width == 0 || height == 0 || halfWidth < 0 || halfHeight < 0) {
		throw std::invalid_argument("Invalid input parameters for GaussianBlur.");
	}

	std::vector<PixelRGBA> temp(src.size());
	dst = temp;

	double inverseSigma = 1.0 / strn;

	for (int i = 0; i < src.size(); i++) {
		int x = i % width;
		int y = i / width;
		for (int yoffset = -halfHeight; yoffset <= halfHeight; yoffset++) {
			int newY = y + yoffset;
			if (newY < 0 || newY >= height) continue;
				for (int xoffset = -halfWidth; xoffset <= halfWidth; xoffset ++) {
					int newX = x + xoffset;
					if (newX < 0 || newX >= width) continue;
					unsigned int postemp = newX + newY * width;
					PixelRGBA srcpixel = src[postemp];
					double distanceSquared = xoffset * xoffset + yoffset * yoffset;
					double weight = std::exp(-distanceSquared / (2 * inverseSigma * inverseSigma));
					temp[i].r += static_cast<uint8_t>(srcpixel.r * weight);
					temp[i].g += static_cast<uint8_t>(srcpixel.g * weight);
					temp[i].b += static_cast<uint8_t>(srcpixel.b * weight);
			}
		}
	}
#else

#endif
}
