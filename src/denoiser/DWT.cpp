#include "denoiser.h"
#include <iostream>

#define doBackward 1
#define doDivide 0

Denoiser::DWT::DecNode::DecNode(unsigned int width, unsigned int height, long layer) : low(nullptr), high(nullptr), width(width), height(height), layer(layer) {
	brightnessData = std::vector<float>(width * height);
}


int Denoiser::DWT::DecNode::DecomposeNode(int wavelet)
{
	int direction = (layer + 1) % 2;
	if (direction != 0 && direction != 1) return 1;

	if (low != nullptr || high != nullptr) return 1;

	unsigned int newWidth;
	unsigned int newHeight;

	int horStride;
	int vertStride;

#if doDivide


	if (direction == 0) {
		newWidth = (width + 3) / 2;
		newHeight = height;
		horStride = 2;
		vertStride = 1;
	}
	else {
		newHeight = (height + 3) / 2;
		newWidth = width;
		horStride = 1;
		vertStride = 2;
	}

	unsigned int newWidth = width;
	unsigned int newHeight = height;

	int horStride;
	int vertStride;

	int verPad;
	int horPad;

	if(direction == 0) {
		newWidth = (width + 4 - 1) / 2;
		horStride = 2;
		vertStride = 1;
		verPad = 0;
		horPad = 4 - 1;
	}
	else {
		newHeight = (height + 4 - 1) / 2;
		horStride = 1;
		vertStride = 2;
		verPad = 4 - 1;
		horPad = 0;
	}

#else

	if (direction == 0) {
		newWidth = (width + 3);
		newHeight = height;
		horStride = 1;
		vertStride = 1;
	}
	else {
		newHeight = (height + 3);
		newWidth = width;
		horStride = 1;
		vertStride = 1;
	}

#endif

	low = std::make_shared<DecNode>(newWidth, newHeight, layer + 1);
	high = std::make_shared<DecNode>(newWidth, newHeight, layer + 1);


	//Low pass first, high pass second

	for (int pass = 0; pass <= 1; pass++) {
		std::span<const double, 4> coefficients = std::span<const double, 4>(pass == 0 ? sym2.dec_lo.data() : sym2.dec_hi.data(), 4);
		std::shared_ptr<DecNode> dst = ((pass == 0) ? low : high);
		for (int y = 0; y < newHeight; y++) {
			for (int x = 0; x < newWidth; x++) {
				double sum = 0;
#if doBackward
				for (int w = 0; w < 4; ++w) {

					int xPos = direction == 0 ? horStride * x - w : x;
					int yPos = direction == 1 ? vertStride * y - w : y;

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

				int dstPos = PosCompose(x, y, newWidth);

				dst->brightnessData[dstPos] = sum;
			}
		}
	}

	width = newWidth;
	height = newHeight;

	brightnessData = std::vector<float>(0);
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

#if doDivide

	if (direction == 0) {
		newWidth = 2 * low->width;
		newHeight = low->height;
	}
	else {
		newHeight = 2 * low->height;
		newWidth = low->width;
	}


	low->brightnessData.resize(oldLowLength * 2);
	high->brightnessData.resize(oldHighLength * 2);

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
			std::memmove(&(low->brightnessData)[index * 2], &(low->brightnessData)[index], newWidth * sizeof(float));
			std::memmove(&(high->brightnessData)[index * 2], &(high->brightnessData)[index], newWidth * sizeof(float));
		}
	}

#else


	if (direction == 0) {
		newWidth = low->width + 3;
		newHeight = low->height;
	}
	else {
		newHeight = low->height + 3;
		newWidth = low->width;
	}

	//upsampling
	//Zero padding horizontally
	if (direction == 0) {
		for (int index = static_cast<int>(oldLowLength - 1); index >= 0; index-=2) {
			(low->brightnessData)[index] = 0;
			(high->brightnessData)[index] = 0;
		}
	}
	else if (direction == 1) {
		for (int index = static_cast<int>(oldLowLength - newWidth - 1);
			index >= 0;
			index -= 2 * newWidth) {
			std::memset(&(low->brightnessData)[index], 0, newWidth * sizeof(float));
			std::memset(&(high->brightnessData)[index], 0, newWidth * sizeof(float));
		}
	}

#endif



	brightnessData = std::vector<float>(newHeight * newWidth, 0.0);

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
		for (int y = 0; y < extendsHeight; y++) {
			for (int x = 0; x < extendsWidth; x++) {
				double sum = 0;
				for (int w = 0; w < 4; ++w) {

					const int delay = 0;
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

				int dstPos = PosCompose(x, y, extendsWidth);

				brightnessData[dstPos] += sum;
			}
		}
	}
      
	width = extendsWidth;
	height = extendsHeight;

	brightnessData = intermediate;

	//brightnessData.resize(newWidth * newHeight);

 //   if (direction == 0) { 
 //       // HORIZONTAL: Crop 3 columns from the left
 //       int startCol = filterDelay;
 //       for (int y = 0; y < newHeight; y++) {
 //           int srcIdx = y * extendsWidth + startCol;
 //           int dstIdx = y * newWidth;
 //           std::memcpy(&brightnessData[dstIdx], &intermediate[srcIdx], newWidth * sizeof(float));
 //       }
 //   } else { 
 //       // VERTICAL: Crop 3 rows from the top
 //       int startRow = filterDelay;
 //       int startMemIdx = startRow * extendsWidth;
 //           
 //       std::memcpy(&brightnessData[0], &intermediate[startMemIdx], newWidth * newHeight * sizeof(float));
 //   }

	low = nullptr;
	high = nullptr;

	width = newWidth;
	height = newHeight;
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
	if (rootNode->DecomposeNode(0) == 0 && rootNode->low->DecomposeNode(0) == 0 && rootNode->high->DecomposeNode(0) == 0) {
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
	if (rootNode->low->RecomposeNode() == 0 && rootNode->high->RecomposeNode() == 0 && rootNode->RecomposeNode(DecNode::ReconMode::Low) == 0) {
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

RGBAImageI Denoiser::DWT::DecTree::GetImageGray(float Y)
{
	RGBAImageI dst(rootNode->width, rootNode->height, 4);

	std::cout << "Rootnode: " << rootNode->width << " * " << rootNode->height << "\n";
	std::cout << "Root Image: " << rootImage.size() << "\n";
	std::cout << "brightnessData : " << rootNode->brightnessData.size() << "\n";
	std::cout << "Dst: " << dst.data.size() << "\n";
	for (int i = 0; i < dst.data.size(); ++i) {
		std::array<float, 3> currPix = YCbCrtoLRGB(rootNode->brightnessData[i] * Y, 0, 0);

		dst.data[i] = PixelRGBA(currPix[0] * 255.0, currPix[1] * 255.0, currPix[2] * 255.0, 255);
	}
	return dst;
}

std::vector<std::array<float, 3>> Denoiser::DWT::DecTree::GetImageYCbCr()
{
	return rootImage;
}
