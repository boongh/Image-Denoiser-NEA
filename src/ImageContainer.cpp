#pragma once

///Implementation of ImageContainer.h
#include <FileReader.h>
#include <QOIFormat.h>
#include "ImageContainer.h"
#include <ImGuiimageloader.h>
#include <CustomWidget.h>
#include <imgui.h>
#include <fstream>
#include <cassert>
#include <span>

//<---Image access token class implementation--->

//Manual release source
void ImageEntry::ImageAccess::Release() { delete(this); }

//Acquire source at initialization
ImageEntry::ImageAccess::ImageAccess(ImageEntry& entrySource) : source(entrySource) { source.useCount.fetch_add(1, std::memory_order_acquire); }

//Release source by end of life
ImageEntry::ImageAccess::~ImageAccess() { source.useCount.fetch_sub(1, std::memory_order_release); }


/// <summary>
/// Initialization of an entry
/// Requires manual LoadImage() to fully access it
/// </summary>
/// <param name="filePath"></param>
ImageEntry::ImageEntry(const std::string& filePath)
	: path(filePath), width(0), height(0), channels(0), TextureLoaded(false), textureID(0),
	status(CompressionStatus::UNLOADED) {
}

std::span<const PixelRGBA> ImageEntry::ReadImageData() const {
	return std::span<const PixelRGBA>(imageData.data.data(), imageData.data.size());
}

///< !-- - ImageEntry Class--->

bool ImageEntry::CheckBound(int x, int y) const {
	return (x >= 0 && x < width && y >= 0 && y < height);
}

int ImageEntry::WriteSpan(int tlx, int tly, std::vector<std::span<PixelRGBA>> src) {
	std::lock_guard lock(lockstate);

	//Check input coherence, all span in the vector must be the same length to form a rectangle
	int widthsrc = src[0].size();
	for (auto s : src) {
		if (s.size() != widthsrc) return -1;
		widthsrc = s.size();
	}
	int heightsrc = src.size();


	//Check if the writing region is valid
    if (!IsFree() || !IsDecompressed()) return -1;
	if (!CheckBound(tlx, tly) || !CheckBound(tlx + widthsrc, tly + heightsrc)) return -1;

	//memcpy each span onto the bitmap data
	for (int j = 0; j < height; ++j) {
		std::memcpy(imageData.data.data() + tlx + (tly + j) * width, src[j].data(), width * sizeof(PixelRGBA));
	}
}

int ImageEntry::SetPixel(int x, int y, int r, int g, int b, int a){
	std::lock_guard lock(lockstate);
	AcquireRead();

	try {

		if (CheckBound(x, y)) {
			return -1; // Out of bounds
		} else if(!IsDecompressed()) {
			return -2; // Image not fully in memory
		}

		imageData[(y * width + x)] = PixelRGBA(r, g, b, a);
	}
	catch (std::exception e) {
		ReleaseRead();
		return -1;
	}
	return 0; // Success
}


int ImageEntry::GetPixel(int x, int y, PixelRGBA& p) const {
	assert(IsDecompressed());

	//WIP

	return 0;
}


int ImageEntry::SetPixel(int x, int y, const PixelRGBA& p) {
	std::lock_guard lock(lockstate);
	AcquireRead();

	try {

		if (CheckBound(x, y)) {
			return -1; // Out of bounds
		}
		else if (!IsDecompressed()) {
			return -2; // Image not fully in memory
		}

		imageData[(y * width + x)] = p;
	}
	catch (std::exception e) {
		ReleaseRead();
		return -1;
	}
	return 0; // Success
}

unsigned int ImageEntry::GetTextureID() const {
	ImageAccess a = AcquireRead();
	return textureID;
}



/// Currently can only load 8bit image due to limitation of the FileReader Library
int ImageEntry::LoadImage() {

	std::lock_guard<std::mutex> lock(lockstate); // Ensure thread safety

	// Check if the image is already loaded or in use, then loading again should not be allowed
	if (IsLoaded()) return 0; // Already loaded
	if (useCount > 0) return -2; // Image is already in use, cannot load

	try {
		FileReader::ReadImage(path, imageData);

		width = imageData.width;
		height = imageData.height;
		channels = imageData.channels;

		status = CompressionStatus::DECOMPRESSED; // Set the status to loaded
		return 0; // Success

	} catch (const std::exception&) {
		return -1; // Error loading image
	}
}

int ImageEntry::LoadTexture() {
	ImageAccess access = AcquireRead();
	if (TextureLoaded) return -2; //Image already loaded
	LoadImage_s(textureID, imageData.data.data(), width, height, channels);
	TextureLoaded = true;
	return 0;
}

int ImageEntry::UnloadImage() {
	std::lock_guard lock(lockstate);

	if (!IsFree()) return -2; //Image is still in use

	status = CompressionStatus::UNLOADED;
	imageData.Clear();
	imageDataCompressed.clear();
	width = 0;
	height = 0;
	channels = 0;

	return 0;
}

int ImageEntry::UnloadTexture() {
	if (!TextureLoaded) return -2; // Texture not currently loaded
	UnloadImage_s(textureID);
	TextureLoaded = false;
	return 0;
}

int ImageEntry::CompressImageData() {
	std::lock_guard lock(lockstate);
	if (!IsFree()) return -2;


	QOICompress(std::span<uint8_t>(reinterpret_cast<uint8_t*>(imageData.data.data()), width * height * sizeof(PixelRGBA)), imageDataCompressed, width, height, channels);

	status = CompressionStatus::COMPRESSED;
	//Clear uncompressed image
	imageData.Clear();

	return -1;
}

int ImageEntry::DecompressImageData() {
	std::lock_guard lock(lockstate);
	if (!IsFree()) return -2;

	QOIDecompress(std::span<uint8_t>(imageDataCompressed.data(), imageDataCompressed.size()), imageData.data, width, height);

	status = CompressionStatus::DECOMPRESSED;
	//Clear compressed image
	imageDataCompressed.clear();

	return -1;
}



ImageEntry::ImageAccess ImageEntry::AcquireRead() const { // Increment the use count when the image is being used
	assert(IsDecompressed());
	useCount.fetch_add(1, std::memory_order_acquire);

	// The ImageAccess constructor requires a non-const reference to ImageEntry.
	// But this method is const, so we need to cast away constness.
	// This is safe here because ImageAccess only reads data and manages useCount.
	
	return ImageAccess(const_cast<ImageEntry&>(*this));
}

// Decrement the use count when the image is no longer being used 
void ImageEntry::ReleaseRead() const { useCount.fetch_sub(1, std::memory_order_release); }

// Check if the image is loaded
bool ImageEntry::IsLoaded() const { return status != CompressionStatus::UNLOADED; }

// Check if the image is decompressed
bool ImageEntry::IsDecompressed() const { return status == CompressionStatus::DECOMPRESSED; }

//Check if the image has no-one reading
bool ImageEntry::IsFree() const { return useCount.load() == 0; }

//Get file path of the image
std::string ImageEntry::GetFilePath() const { return path; }

//Getters for metadata
int ImageEntry::GetWidth() const { return width; }
int ImageEntry::GetHeight() const { return height; }
int ImageEntry::GetChannels() const { return channels; }

int ImageManager::GetImageCount() { return imageEntries.size(); }

int ImageManager::ImportFromFile(std::string path) {
	ImageEntry* newImage = new ImageEntry(path);
	imageEntries.push_back(newImage);
	return 0;
}

int ImageManager::LazyLoadImage(int index) {
	imageEntries[index]->LoadImage();
	return 0;
}

void ImageManager::Compress(int index) { imageEntries[index]->CompressImageData(); }

void ImageManager::Decompress(int index) { imageEntries[index]->DecompressImageData(); }

void ImageManager::LoadGPU(int index) { imageEntries[index]->LoadTexture(); }

void ImageManager::UnloadGPU(int index) { imageEntries[index]->UnloadTexture(); }

void ImageManager::DisplayImage(int index) {
	ImageEntry* entry = imageEntries[index];
	if (entry->TextureLoaded) LoadImageTooltipWidget(entry->GetTextureID(), ImVec2(entry->width, entry->height), ImVec2(64, 64), ImVec4(0, 0, 0, 0), 4);
}

ImageEntry::ImageAccess ImageManager::ReadImage(int id) { return imageEntries[id]->AcquireRead(); }

std::string ImageManager::GetName(int id) const { return imageEntries[id]->GetFilePath(); }

ImVec2 ImageManager::GetDim(int id) const
{
	ImageEntry* image = imageEntries[id];
	return ImVec2(image->GetWidth(), image->GetHeight());
}
