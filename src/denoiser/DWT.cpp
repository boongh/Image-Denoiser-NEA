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
	if ((direction == 0 && width % 2 != 0) || (direction == 1 && height % 2 != 0)) return 1;
	if (direction != 0 && direction != 1) return 1;


	if (low != nullptr || high != nullptr) return 1;

#if doDivide

	unsigned int newWidth = direction == 0 ? width / 2 : width;
	unsigned int newHeight = direction == 1 ? height / 2 : height;

	int horStride = direction == 1 ? 1 : 2;
	int vertStride = direction == 0 ? 1 : 2;

#else

	unsigned int newWidth = width;
	unsigned int newHeight = height;

	int horStride = 1;
	int vertStride = 1;

#endif

	low = std::make_shared<DecNode>(newWidth, newHeight, layer + 1);
	high = std::make_shared<DecNode>(newWidth, newHeight, layer + 1);


	//Low pass first, high pass second

	for (int pass = 0; pass <= 1; pass++) {
		std::span<const double, 4> coefficients = std::span<const double, 4>(pass == 0 ? sym2.dec_lo.data() : sym2.dec_hi.data(), 4);
		std::shared_ptr<DecNode> dst = ((pass == 0) ? low : high);
		for (int y = 0; y < height; y += vertStride) {
			for (int x = 0; x < width; x += horStride) {
				double sum = 0;
#if doBackward
				for (int w = 0; w < 4; ++w) {

					int xPos = direction == 0 ? x - w : x;
					int yPos = direction == 1 ? y - w : y;

					xPos = (xPos < 0) ? -xPos - 1 : (xPos >= width ? 2 * width - xPos - 1 : xPos);
					yPos = (yPos < 0) ? -yPos - 1 : (yPos >= height ? 2 * height - yPos - 1 : yPos);	

					int position = PosCompose(xPos, yPos, width);
					sum += coefficients[w] * brightnessData[position];
				}

#else// 1
				for (int w = 0; w < 4; ++w) {

					int xPos = direction == 0 ? x + w : x;
					int yPos = direction == 1 ? y + w : y;

					xPos = xPos >= width ? width - ((xPos - width) + 1) : xPos;
					yPos = yPos >= height ? height - ((yPos - height) + 1) : yPos;

					int position = PosCompose(xPos, yPos, width);
					sum += coefficients[w] * brightnessData[position];
				}

#endif

#if doDivide
				int dstPos = PosCompose(
					direction == 0 ? x >> 1 : x,
					direction == 1 ? y >> 1 : y,
					newWidth);
#else
				int dstPos = PosCompose(x, y, newWidth);
#endif
				dst->brightnessData[dstPos] = sum;
			}
		}
	}

	brightnessData = std::vector<float>(width * height, 0);
	return 0;
}

int Denoiser::DWT::DecNode::RecomposeNode(ReconMode mode)
{
	int direction = low->layer % 2;

	if (direction != 0 && direction != 1) {
		return 1;
	}
	if (low == nullptr || high == nullptr) return 1;

	unsigned oldLowLength = low->brightnessData.size();
	unsigned oldHighLength = high->brightnessData.size();

	unsigned int newWidth = width;
	unsigned int newHeight = height;

	brightnessData = std::vector<float>(newHeight * newWidth, 0.0);


#if doDivide

	low->brightnessData.resize(low->brightnessData.size() * 2);
	high->brightnessData.resize(high->brightnessData.size() * 2);

	//upsampling
	//Zero padding horizontally
	if (direction == 0) {
		for (int i = oldLowLength - 1; i >= 0; --i) {
			std::swap(low->brightnessData[i], low->brightnessData[2 * i]);
			std::swap(high->brightnessData[i], high->brightnessData[2 * i]);
		}
	}
	else if (direction == 1) {
		for (int index = static_cast<int>(oldLowLength - newWidth - 1);
			index >= 0;
			index -= static_cast<int>(newWidth)) {
			std::memcpy(&(low->brightnessData)[index * 2], &(low->brightnessData)[index], newWidth * sizeof(float));
			std::memcpy(&(high->brightnessData)[index * 2], &(high->brightnessData)[index], newWidth * sizeof(float));
			std::memset(&(low->brightnessData)[index], 0, newWidth * sizeof(float));
			std::memset(&(high->brightnessData)[index], 0, newWidth * sizeof(float));
		}
	}

#else

	//upsampling
	//Zero padding horizontally
	if (direction == 0) {
		for (int index = static_cast<int>(oldLowLength - 1); index >= 0; index-=2) {
			/*(low->brightnessData)[index] = 0;
			(high->brightnessData)[index] = 0;*/
		}
	}
	else if (direction == 1) {
		for (int index = static_cast<int>(oldLowLength - newWidth - 1);
			index >= 0;
			index -= 2 * newWidth) {
			/*std::memset(&(low->brightnessData)[index], 0, newWidth * sizeof(float));
			std::memset(&(high->brightnessData)[index], 0, newWidth * sizeof(float));*/
		}
	}

#endif



	//zero padding vertically
	//Copy each row to their index * 2
	//Zeros the row out afterward

	//Low pass first, high pass second
	int first = 0, stop = 1;
	if (mode == ReconMode::Low) {
		stop = 0;
	}
	if (mode == ReconMode::High) {
		first = 1;
	}
	for (int pass = first; pass <= stop; pass++) {
		std::span<const double, 4> coefficients = std::span<const double, 4>(pass == 0 ? sym2.rec_lo.data() : sym2.rec_hi.data(), 4);
		std::vector<float> src = pass == 0 ? low->brightnessData : high->brightnessData;
		for (int y = 0; y < newHeight; y++) {
			for (int x = 0; x < newWidth; x++) {
				double sum = 0;
				for (int w = 0; w < 4; ++w) {

					// FIX: Compensate for the sym2 wavelet's delay
					const int delay = 1;
					int read_x = x;
					int read_y = y;

					if (direction == 0) {
						read_x += delay;
					}
					else {
						read_y += delay;
					}

#if doBackward
					int xPos = direction == 0 ? read_x - w : read_x;
					int yPos = direction == 1 ? read_y - w : read_y;

#else// 1
					int xPos = direction == 0 ? read_x + w : read_x;
					int yPos = direction == 1 ? read_y + w : read_y;

#endif
					xPos = (xPos < 0) ? - xPos - 1 : (xPos >= width ? 2 * width - xPos - 1 : xPos);
					yPos = (yPos < 0) ? - yPos - 1 : (yPos >= height ? 2 * height - yPos - 1 : yPos);

					int position = PosCompose(xPos, yPos, width);
					sum += coefficients[w] * src[position];
				}

				int dstPos = PosCompose(x, y, newWidth);

				if (dstPos == 0) {
					std::cout << "dstPos: " << dstPos << "\n";
				}
				brightnessData[dstPos] += sum;
			}
		}
	}


	low = nullptr;
	high = nullptr;
	return 0;
}

Denoiser::DWT::DecTree::DecTree() : expanded(false), rootNode(nullptr)
{

}

Denoiser::DWT::DecTree::DecTree(std::span<const PixelRGBA> src, unsigned int width, unsigned int height) {
	expanded = false;
	rootAlpha = std::vector<int>(width * height);
	rootImage = std::vector<std::array<float, 3>>(width * height);
	rootNode = std::make_shared<DecNode>(width, height, -1);

	for (int index = 0; index < height * width; ++index) {
		//Convert to normalized YCbCr from normalized RGB
		rootImage[index] = LRGBtoYCbCr(src[index].r / 255.0, src[index].g / 255.0, src[index].b / 255.0);
		rootAlpha[index] = src[index].a;
		(rootNode->brightnessData)[index] = rootImage[index][0];
	}
}

int Denoiser::DWT::DecTree::ExpandTree()
{
	if (rootNode->DecomposeNode(0)) {
		// == 0 && rootNode->low->DecomposeNode(0) == 0 && rootNode->high->DecomposeNode(0) == 0
		expanded = true;
		return 0;
	}
	else {
		return 1;
	}
}

int Denoiser::DWT::DecTree::CollapseTree(DecNode::ReconMode mode)
{
	if (rootNode->RecomposeNode(DecNode::ReconMode::High) == 0) {
		//rootNode->low->RecomposeNode() == 0 && rootNode->high->RecomposeNode() == 0 && 
		expanded = false;
		return 0;
	}
	else {
		return 1;
	}
}

RGBAImageI Denoiser::DWT::DecTree::GetImageRGB(float Y, float Cb, float Cr, float r, float g, float b) {
	RGBAImageI dst(rootNode->width, rootNode->height, 4);

	std::cout << "Rootnode: " << rootNode->width << " * " << rootNode->height << "\n";
	std::cout << "Root Image: " << rootImage.size() << "\n";
	std::cout << "brightnessData : " << rootNode->brightnessData.size() << "\n";
	std::cout << "Dst: " << dst.data.size() << "\n";
	for (int i = 0; i < dst.data.size(); ++i) {
		std::array<float, 3> currPix = YCbCrtoLRGB(rootNode->brightnessData[i] * Y, rootImage[i][1] * Cb, rootImage[i][2] * Cr);

		dst.data[i] = PixelRGBA(currPix[0] * 255.0 * r, currPix[1] * 255.0 * g, currPix[2] * 255.0 * b, rootAlpha[i]);
	}
	return dst;
}

std::vector<std::array<float, 3>> Denoiser::DWT::DecTree::GetImageYCbCr()
{
	return rootImage;
}
