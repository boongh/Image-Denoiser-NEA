#include "denoiser.h"

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

			
#if 1
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
					if (actX < 0 || actX >= width || actY < 0 || actY >= height) continue;
					postemp = PosCompose(x + xPos, y + yPos, width);
					PixelRGBA srcpixel = src[postemp];
					sumR += srcpixel.r;
					sumG += srcpixel.g;
					sumB += srcpixel.b;
					cum++;
				}
			}
#endif // 0

			if (cum == 0) {
				printf("Latest pixel %d before crash (%d, %d, %d) with count %d", pixelpos, sumR, sumG, sumB, cum);
			}

			pinit = src[pixelpos];
			sumR /= cum; sumG /= cum;  sumB /= cum;

			dst[pixelpos] = PixelRGBA(
				lerp<uint8_t>(sumR, pinit.r, strn),
				lerp<uint8_t>(sumG, pinit.g, strn),
				lerp<uint8_t>(sumB, pinit.b, strn),
				pinit.a
			);

#ifdef DEBUG	
			if (pixelpos % 10000 == 0) {
				printf("Processing pixel %d : %d\n", pixelpos, static_cast<int>(src.size()));
				printf("Values are (%d, %d, %d, %d)\n\n", dst[pixelpos].r, dst[pixelpos].g, dst[pixelpos].b, dst[pixelpos].a);
			}
#endif // DEBUG

		}
		return dst;
	}
	catch (std::exception e) {
		return dst;
	}
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
