#pragma once

#pragma region Includes

#include <ImGuiimageloader.h>
#include <CustomWidget.h>
#include <imgui.h>

#include <fstream>
#include <cassert>

#include <FileReader.h>
#include <QOIFormat.h>
#include "ImageContainer.h"

#ifndef  STB_IMAGE_IMPLEMENTATION 
#include <stb_image.h>
#define  STB_IMAGE_IMPLEMENTATION 
#endif // ! STB_IMAGE_IMPLEMENTATION 

#include <span>


#pragma endregion


#pragma region Image Manager
//<---Image Manager class implementation--->

size_t ImageManager::GetImageCount() { return imageEntries.size(); }

int ImageManager::ImportFromFile(std::string path) {
	auto newImage = std::make_shared<ImageEntry>(path);
	imageEntries.push_back(newImage);
	return 0;
}

int ImageManager::ImportFromSpan(std::span<PixelRGBA> src, unsigned int width, unsigned int height, std::string name) {
	auto newImage = std::make_shared<ImageEntry>(src, width, height, name);
	imageEntries.push_back(newImage);
	return 0;
}

int ImageManager::LazyLoadImage(int index) {
	imageEntries[index]->LoadImage();
	return 0;
}

void ImageManager::Compress(int index) { imageEntries[index]->CompressImageData(); }
void ImageManager::Decompress(int index) { imageEntries[index]->DecompressImageData(); }

std::shared_ptr<ImageRenderer> ImageManager::CreateRenderer(int index) {
	auto entry = imageEntries[index];
	auto renderer = std::make_shared<ImageRenderer>(entry);
	imageRenderers.insert({ entry, renderer });
	return renderer;
}

std::shared_ptr<ImageRenderer> ImageManager::GetRenderer(int index) {
	if (index >= 0 && index < imageEntries.size()) {
		auto rval = imageRenderers.find(imageEntries[index]);
		if (rval == imageRenderers.end()) return nullptr;
		return rval->second;
	}
	return nullptr;
}

std::shared_ptr<ImageRenderer> ImageManager::GetRenderer(std::shared_ptr<ImageEntry> imageEntry) {
	auto rval = imageRenderers.find(imageEntry);
	if (rval == imageRenderers.end()) return nullptr;
	return rval->second;
}

void ImageManager::DestroyRenderer(int index) {
	imageRenderers.erase(imageEntries[index]);
}

std::shared_ptr<ImageEntry> ImageManager::GetImage(int id) { return imageEntries[id]->AcquireRead(); }
std::string ImageManager::GetName(int id) const { return imageEntries[id]->GetFilePath(); }

ImVec2 ImageManager::GetDim(int id) const {
	auto image = imageEntries[id];
	return ImVec2(static_cast<float>(image->GetWidth()), static_cast<float>(image->GetHeight()));
}

#pragma endregion