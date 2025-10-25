#include "denoiser.h"
#include <iostream>

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
		rootImage[index] = LRGBtoYCbCr(src[index].r / 255.0f, src[index].g / 255.0f, src[index].b / 255.0f);
		rootAlpha[index] = src[index].a;
		(rootNode->brightnessData)[index] = rootImage[index][0];
	}
}

void TreeExpandHelper(std::shared_ptr<Denoiser::DWT::DecNode> node, int currentLevel, int targetLevel) {
	if (currentLevel >= targetLevel) {
		return;
	}
	if (node->low == nullptr && node->high == nullptr) {
		node->DecomposeNode(0);
	}
	TreeExpandHelper(node->low, currentLevel + 1, targetLevel);
	TreeExpandHelper(node->high, currentLevel + 1, targetLevel);
}

void TreeCollapseHelper(std::shared_ptr<Denoiser::DWT::DecNode> node, int currentLevel, Denoiser::DWT::DecNode::ReconMode mode = Denoiser::DWT::DecNode::ReconMode::Full) {
	if (node->low != nullptr && node->high != nullptr) {
		TreeCollapseHelper(node->low, currentLevel + 1, Denoiser::DWT::DecNode::ReconMode::Full);
		TreeCollapseHelper(node->high, currentLevel + 1, Denoiser::DWT::DecNode::ReconMode::Full);
		node->RecomposeNode(mode);
	}
}

int Denoiser::DWT::DecTree::ExpandTree()
{
	const int decimationLevel = 3;
	TreeExpandHelper(rootNode, -1, decimationLevel);
	return 0;
}

int Denoiser::DWT::DecTree::CollapseTree(DecNode::ReconMode mode)
{
	TreeCollapseHelper(rootNode, -1, Denoiser::DWT::DecNode::ReconMode::Full);
	return 0;
}

RGBAImageI Denoiser::DWT::DecTree::GetImageRGB(float Y, float Cb, float Cr, float r, float g, float b) {
	RGBAImageI dst(rootNode->width, rootNode->height, 4);

	std::cout << "Rootnode: " << rootNode->width << " * " << rootNode->height << "\n";
	std::cout << "Root Image: " << rootImage.size() << "\n";
	std::cout << "brightnessData : " << rootNode->brightnessData.size() << "\n";
	std::cout << "Dst: " << dst.data.size() << "\n";
	for (int i = 0; i < dst.data.size(); ++i) {
		std::array<float, 3> currPix = YCbCrtoLRGB(rootNode->brightnessData[i] * Y, rootImage[i][1] * Cb, rootImage[i][2] * Cr);

		dst.data[i] = PixelRGBA(currPix[0] * 255.0f * r, currPix[1] * 255.0f * g, currPix[2] * 255.0f * b, rootAlpha[i]);
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
