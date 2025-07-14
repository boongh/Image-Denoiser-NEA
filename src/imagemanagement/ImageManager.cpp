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
#include <set>


#pragma endregion


#pragma region Image Manager
//<---Image Manager class implementation--->

size_t ImageManager::GetImageCount() { return imageEntries.size(); }

std::vector<std::shared_ptr<ImageEntry>>::iterator ImageManager::begin() {
	return imageEntries.begin();
}

std::vector<std::shared_ptr<ImageEntry>>::const_iterator ImageManager::begin() const {
	return imageEntries.begin();
}

std::vector<std::shared_ptr<ImageEntry>>::const_iterator ImageManager::cbegin() const {
	return imageEntries.cbegin();
}

std::vector<std::shared_ptr<ImageEntry>>::iterator ImageManager::end() {
	return imageEntries.end();
}
std::vector<std::shared_ptr<ImageEntry>>::const_iterator ImageManager::end() const{ 
	return imageEntries.end();
}
std::vector<std::shared_ptr<ImageEntry>>::const_iterator ImageManager::cend() const{ 
	return imageEntries.cend();
}

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

int ImageManager::LazyLoadImage(std::shared_ptr<ImageEntry> imageEntry) {
	auto it = std::find(imageEntries.begin(), imageEntries.end(), imageEntry);
	if (it != imageEntries.end()) {
		(*it)->LoadImage();
		return 0;
	}
	return -1; // Image not found
}

int ImageManager::UnloadImage(int index) {
	imageEntries.erase(imageEntries.begin() + index);
	return 0;
}

int ImageManager::UnloadImage(std::set<std::shared_ptr<ImageEntry>> scheduledDeletion) {

	auto removeitem = std::remove_if(imageEntries.begin(), imageEntries.end(),
		[&](const std::shared_ptr<ImageEntry>& entry) {
			return scheduledDeletion.find(entry) != scheduledDeletion.end();
		});
	imageEntries.erase(removeitem, imageEntries.end());
	return 0;
}

ImageEntry::CompressionStatus ImageManager::GetStatus(int index) const {
	return imageEntries[index]->GetStatus();
}

void ImageManager::Compress(int index) { imageEntries[index]->CompressImageData(); }
void ImageManager::Decompress(int index) { imageEntries[index]->DecompressImageData(); }

std::shared_ptr<ImageRenderer> ImageManager::CreateRenderer(int index) {
	auto entry = imageEntries[index];
	auto renderer = std::make_shared<ImageRenderer>(entry);
	imageRenderers.insert({ entry, renderer });
	return renderer;
}

std::shared_ptr<ImageRenderer> ImageManager::CreateRenderer(std::shared_ptr<ImageEntry> item) {
	auto it = std::find(imageEntries.begin(), imageEntries.end(), item);
	if(it != imageEntries.end()) {
		auto renderer = std::make_shared<ImageRenderer>(item);
		imageRenderers.insert({ item, renderer });
		return renderer;
	}
	else {
		return nullptr; // ImageEntry not found
	}
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

void ImageManager::DestroyRenderer(std::shared_ptr<ImageEntry> imageEntry) {
	auto rval = imageRenderers.find(imageEntry);
	if (rval != imageRenderers.end()) {
		imageRenderers.erase(rval);
	}
}

std::shared_ptr<ImageEntry> ImageManager::GetImage(int id) { return (id >= 0 && id < imageEntries.size()) ? imageEntries[id]->AcquireRead() : nullptr; }
std::string ImageManager::GetPath(int id) const { return imageEntries[id]->GetFilePathString(); }

std::filesystem::path ImageManager::GetPath_path(int id) const
{
	return imageEntries[id]->GetFilePath_path();
}

ImVec2 ImageManager::GetDim(int id) const {
	auto image = imageEntries[id];
	return ImVec2(static_cast<float>(image->GetWidth()), static_cast<float>(image->GetHeight()));
}

#pragma endregion