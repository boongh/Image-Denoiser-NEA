#include "denoiser.h"
#include <iostream>


#define doBackward 1
#define doDivide 1

Denoiser::DWT::DecNode::DecNode(unsigned int width, unsigned int height, long layer) : low(nullptr), high(nullptr), width(width), height(height), layer(layer) {
	brightnessData = std::vector<float>(width * height);
}


int Denoiser::DWT::DecNode::DecomposeNode(int wavelet)
{
	int direction = (layer + 1) % 2;
	if (direction != 0 && direction != 1) return 1;


	if (low != nullptr || high != nullptr) return 1;

	unsigned int convolvedWidth;
	unsigned int convolvedHeight;

	int horStride;
	int vertStride;

#if doDivide


	if (direction == 0) {
		convolvedWidth = (width + 4) / 2;
		convolvedHeight = height;
		horStride = 2;
		vertStride = 1;
	}
	else {
		convolvedHeight = (height + 4) / 2;
		convolvedWidth = width;
		horStride = 1;
		vertStride = 2;
	}

#else

	if (direction == 0) {
		convolvedWidth = (width + 3);
		convolvedHeight = height;
		horStride = 1;
		vertStride = 1;
	}
	else {
		convolvedHeight = (height + 3);
		convolvedWidth = width;
		horStride = 1;
		vertStride = 1;
	}

#endif

	low = std::make_shared<DecNode>(convolvedWidth, convolvedHeight, layer + 1);
	high = std::make_shared<DecNode>(convolvedWidth, convolvedHeight, layer + 1);


	//Low pass first, high pass second
	for (int pass = 0; pass <= 1; pass++) {
		const std::array<const double, 4> coefficients = pass == 0 ? sym2.dec_lo : sym2.dec_hi;
		std::shared_ptr<DecNode> dst = ((pass == 0) ? low : high);
		for (unsigned int y = 0; y < convolvedHeight; y++) {
			for (unsigned int x = 0; x < convolvedWidth; x++) {
				double sum = 0;

				for (int w = 0; w < 4; ++w) {

					int xPos = direction == 0 ? horStride * x - w : x;
					int yPos = direction == 1 ? vertStride * y - w : y;

					xPos = (xPos < 0) ? -xPos - 1 : (xPos >= width ? 2 * width - xPos - 1 : xPos);
					yPos = (yPos < 0) ? -yPos - 1 : (yPos >= height ? 2 * height - yPos - 1 : yPos);

					int position = PosCompose(xPos, yPos, width);
					sum += coefficients[w] * brightnessData[position];
				}

				int dstPos = PosCompose(x, y, convolvedWidth);

				dst->brightnessData[dstPos] = sum;
			}
		}
	}

	brightnessData = std::vector<float>(0);
	return 0;
}

int Denoiser::DWT::DecNode::RecomposeNode(ReconMode mode)
{
	const int filterLength = 4;
	int direction = low->layer % 2;

	if (direction != 0 && direction != 1) {
		return 1;
	}
	if (low == nullptr || high == nullptr)
		return 1;

	unsigned oldLowLength = low->brightnessData.size();
	unsigned oldHighLength = high->brightnessData.size();

	unsigned int convolvedWidth;
	unsigned int convolvedHeight;

	unsigned upSampledWidth;
	unsigned upSampledHeight;

#if doDivide

	if (direction == 0) {
		upSampledHeight = low->height;
		upSampledWidth = low->width * 2;
		convolvedWidth = upSampledWidth + filterLength - 1;
		convolvedHeight = upSampledHeight;
	}
	else {
		upSampledHeight = low->height * 2;
		upSampledWidth = low->width;
		convolvedHeight = upSampledHeight + filterLength - 1;
		convolvedWidth = upSampledWidth;
	}


	low->brightnessData.resize(upSampledWidth * upSampledHeight);
	high->brightnessData.resize(upSampledWidth * upSampledHeight);

	//upsampling
	//Zero padding horizontally
	if (direction == 0) {
		for (int i = PosCompose(low->width - 1, upSampledHeight - 1, low->width); i >= 0; --i) {

			std::swap(low->brightnessData[2 * i], low->brightnessData[i]);
			std::swap(high->brightnessData[2 * i], high->brightnessData[i]);

			/*low->brightnessData[2 * i] = low->brightnessData[i];
			high->brightnessData[2 * i] = high->brightnessData[i];
			low->brightnessData[2 * i + 1] = 0.0f;
			high->brightnessData[2 * i + 1] = 0.0f;*/
		}
	}
	else if (direction == 1) {
		for (int index = PosCompose(0, low->height - 1, upSampledWidth);
			index >= 0;
			index -= static_cast<int>(upSampledWidth)) {
			std::memmove(&(low->brightnessData)[index * 2], &(low->brightnessData)[index], width * sizeof(float));
			std::memmove(&(high->brightnessData)[index * 2], &(high->brightnessData)[index], width * sizeof(float));
			std::memset(&(high->brightnessData)[index], 0, width * sizeof(float));
			std::memset(&(low->brightnessData)[index], 0, width * sizeof(float));
		}
	}

#else


	if (direction == 0) {
		convolvedWidth = low->width + 3;
		convolvedHeight = low->height;
		upSampledHeight = low->height;
		upSampledWidth = low->width;
	}
	else {
		convolvedHeight = low->height + 3;
		convolvedWidth = low->width;
		upSampledWidth = low->width;
		upSampledHeight = low->height;
	}

	//upsampling
	//Zero padding horizontally
	if (direction == 0) {
		for (int index = static_cast<int>(oldLowLength - 1); index >= 0; index -= 2) {
			(low->brightnessData)[index] = 0;
			(high->brightnessData)[index] = 0;
		}
	}
	else if (direction == 1) {
		for (int index = static_cast<int>(oldLowLength - low->width - 1);
			index >= 0;
			index -= 2 * low->width) {
			std::memset(&(low->brightnessData)[index], 0, low->width * sizeof(float));
			std::memset(&(high->brightnessData)[index], 0, low->width * sizeof(float));
		}
	}

#endif

	std::vector<float> intermediate = std::vector<float>(convolvedHeight * convolvedWidth, 0.0);

	//Low pass first, high pass second
	int first = 0, stop = 1;
	if (mode == ReconMode::Low) {
		stop = 0;
	}
	if (mode == ReconMode::High) {
		first = 1;
	}

	for (int pass = first; pass <= stop; pass++) {
		const std::array<const double, 4> coefficients = (pass == 0) ? sym2.rec_lo  : sym2.rec_hi;
		std::vector<float> src = pass == 0 ? low->brightnessData : high->brightnessData;
		for (int y = 0; y < convolvedHeight; y++) {
			for (int x = 0; x < convolvedWidth; x++) {
				double sum = 0;
				for (int w = 0; w < filterLength; ++w) {

#if doBackward
					int xPos = x - w * (direction == 0);
					int yPos = y - w * (direction == 1);

#else// 1
					int xPos = direction == 0 ? read_x + w : read_x;
					int yPos = direction == 1 ? read_y + w : read_y;
#endif

					xPos = (xPos < 0) ? -xPos - 1 : (xPos >= upSampledWidth ? 2 * upSampledWidth - xPos - 1 : xPos);
					yPos = (yPos < 0) ? -yPos - 1 : (yPos >= upSampledHeight ? 2 * upSampledHeight - yPos - 1 : yPos);

					int position = PosCompose(xPos, yPos, upSampledWidth);

					if (position < 0 || position >= src.size()) {
						std::cout << "Error: position out of bounds in DWT recomposition.\n";
						return 1;
					}

					sum += coefficients[w] * src[position];
				}

				int dstPos = PosCompose(x, y, convolvedWidth);

#ifdef DEBUG
				if (dstPos < 0 || dstPos >= intermediate.size()) {
					std::cout << "Error: dstPos out of bounds in DWT recomposition.\n";
					return 1;
				}
#endif

				intermediate[dstPos] += sum;
			}
		}
	}


	low = nullptr;
	high = nullptr;

	brightnessData = std::vector<float>(width * height);


	int offset = filterLength - 1;
	//horizontal crop
	//skips the first and the last L - 1 pixels each row
	//RowLength = length - (L - 1) * 2
	if (direction == 0) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int srcPos = PosCompose(x + offset, y, convolvedWidth);
				int dstPos = PosCompose(x, y, width);
				brightnessData[dstPos] = intermediate[srcPos];
			}
		}

	}
	//Vertical crop
	//Skips the first and the last L-1 pixels of each column
	//ColLength = length - (L - 1) * 2
	else if (direction == 1) {
		int srcPos = PosCompose(0, offset, convolvedWidth);
		memmove(&(brightnessData[0]), &(intermediate[srcPos]), width * height * sizeof(float));
	}

	return 0;
}

