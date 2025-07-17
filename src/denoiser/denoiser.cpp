#include "denoiser.h"
#include <unordered_map>
#include <Eigen/Dense>

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
				lerp<uint8_t>(sumR, pinit.r, strn),
				lerp<uint8_t>(sumG, pinit.g, strn),
				lerp<uint8_t>(sumB, pinit.b, strn),
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
	std::unordered_map<int, double> cacheDistAtten;

	for (int x = 0; x <= halfWidth; x++) {
		for(int y = 0; y <= halfHeight; y++) {
			int dist = x * x + y * y;
			if (cacheDistAtten.find(dist) == cacheDistAtten.end()) {
				cacheDistAtten[dist] = Bilateral::RangeAttenuation(dist, inverseSpatial);
			}
		}
	}

	constexpr double INVERSECOLOR = 1 / 255.0;
	double INVERSEWIDTH = 1 / width;
	double INVERSEHEIGHT = 1 / height;
	int loopCount = 0;

	for (int pixelnum = 0; pixelnum < src.size(); pixelnum++) {
		int xPos = 0;
		int yPos = 0;

		double sumR = 0;
		double sumG = 0;
		double sumB = 0;


		double sumW = 0;

		PixelRGBA pinit = src[pixelnum];

		MathVector<float, 3> pinitVecRGB(
			(float)pinit.r * INVERSECOLOR * inverseIntensity,
			(float)pinit.g * INVERSECOLOR * inverseIntensity,
			(float)pinit.b * INVERSECOLOR * inverseIntensity
		);

		// Decompose pixel position
		PosDecompose(pixelnum, width, height, &xPos, &yPos);
		for (int x = -halfWidth; x <= halfWidth; x++) {

			double xsquared = x * x;
			for (int y = -halfHeight; y <= halfHeight; y++) {
				int actX = x + xPos;
				int actY = y + yPos;

				// prevents invalid location
				if (actX < 0 || actX >= width || actY < 0 || actY >= height) continue;

				unsigned int postemp = PosCompose(actX, actY, width);
				PixelRGBA srcpixel = src[postemp];

				int dist = x * x + y * y;
				double distanceFactor = cacheDistAtten[dist];
#if 0
				{

				//Normalize the distance

				double xd = x / width;
				double yd = y / height;
				double rangeDist = xd * xd + yd * yd;

				double rDiffNorm = ((double)pinit.r - srcpixel.r) / 255.0;
				double gDiffNorm = ((double)pinit.g - srcpixel.g) / 255.0;
				double bDiffNorm = ((double)pinit.b - srcpixel.b) / 255.0;

				double DistAttenuation = Bilateral::RangeAttenuation(rangeDist, inverseSpatial);

				// Calculate the weight for each channel
					// Normalize intensity
					double rw = Bilateral::IntensityAttenuation((pinit.r - srcpixel.r) * (pinit.r - srcpixel.r) / 65025.0, inverseIntensity) * DistAttenuation;
					double gw = Bilateral::IntensityAttenuation((pinit.g - srcpixel.g) * (pinit.g - srcpixel.g) / 65025.0, inverseIntensity) * DistAttenuation;
					double bw = Bilateral::IntensityAttenuation((pinit.b - srcpixel.b) * (pinit.b - srcpixel.b) / 65025.0, inverseIntensity) * DistAttenuation;

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

					double omega = Bilateral::VectorAttenuation(pinitVecRGB, srcVecRGB, 1) * distanceFactor;

					sumR += srcpixel.r * omega;
					sumG += srcpixel.g * omega;
					sumB += srcpixel.b * omega;

					sumW += omega;

					loopCount++;

				}
#endif
			}

		}
#ifdef DEBUG

		if (loopCount % 100000 == 0) std::println("{}th loop \n", loopCount);
#endif // DEBUG

		denoisedimage[pixelnum].r = static_cast<uint8_t>(sumR / sumW);
		denoisedimage[pixelnum].g = static_cast<uint8_t>(sumG / sumW);
		denoisedimage[pixelnum].b = static_cast<uint8_t>(sumB / sumW);
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

inline double Denoiser::Bilateral::RangeAttenuation(double distanceSquared, double inverseSD)
{
	return exp(-(distanceSquared) * (0.5 * inverseSD * inverseSD));
}

inline double Denoiser::Bilateral::IntensityAttenuation(double distanceSquared, double inverseSD)
{
	return exp(-(distanceSquared) * (0.5 * inverseSD * inverseSD));
}
