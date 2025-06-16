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


///Implementation of ImageContainer.h

//<---Image Entry class Implementation--->
#pragma region Image Entry


/// <summary>
/// Initialization of an entry
/// Requires manual LoadImage() to fully access it
/// </summary>
/// <param name="filePath"></param>
ImageEntry::ImageEntry(const std::string& filePath)
	: path(filePath), width(0), height(0), channels(0),
	status(CompressionStatus::NOT_LOADED) {
}

ImageEntry::ImageEntry(std::span<PixelRGBA> src, unsigned int w, unsigned int h, std::string name) : 
	width(w), height(h), channels(4), path(name) {
	status = CompressionStatus::DECOMPRESSED;
	if (src.size() < w * h) return;
	imageData = RGBAImageI();
	imageData.Resize(w, h, 4);
	std::memcpy(imageData.data.data(), src.data(), w * h * sizeof(PixelRGBA));
}


std::span<const PixelRGBA> ImageEntry::ReadImageData() const {
	return std::span<const PixelRGBA>(imageData.data.data(), imageData.data.size());
}

///< !-- - ImageEntry Class--->

bool ImageEntry::CheckBound(size_t x, size_t y) const {
	return (x >= 0 && x < width && y >= 0 && y < height);
}

int ImageEntry::WriteSpan(size_t tlx, size_t tly, std::vector<std::span<PixelRGBA>> src) {

	auto lock = std::unique_lock(lockstate);
	if (!IsDecompressed()) return -2;

	size_t widthsrc = src[0].size();
	size_t heightsrc = src.size();

	if (widthsrc == 0 || heightsrc == 0) return -1;

	if (!CheckBound(tlx, tly) || !CheckBound(tlx + widthsrc - 1, tly + heightsrc - 1)) return -1;

	//Check input coherence, all span in the vector must be the same length to form a rectangle
	for (auto s : src) {
		if (s.size() != widthsrc) return -1;
		widthsrc = s.size();
	}


	//memcpy each span onto the bitmap data
	for (int j = 0; j < height; ++j) {
		std::memcpy(imageData.data.data() + tlx + (tly + j) * width, src[j].data(), src[j].size() * sizeof(PixelRGBA));
	}
	return 0;
}

int ImageEntry::SetPixel(int x, int y, int r, int g, int b, int a) {
	std::unique_lock lock(lockstate);

	try {

		if (CheckBound(x, y)) {
			return -1; // Out of bounds
		}
		else if (!IsDecompressed()) {
			return -2; // Image not fully in memory
		}
		else if (!IsFree()) return -2;

		imageData[(y * width + x)] = PixelRGBA(r, g, b, a);
	}
	catch (std::exception e) {
		return -1;
	}
	return 0; // Success
}


int ImageEntry::GetPixel(int x, int y, PixelRGBA& p) const {
	std::shared_lock lock(lockstate);
	assert(IsDecompressed());

	p = imageData.data[x + width * y];

	return 0;
}


int ImageEntry::SetPixel(int x, int y, const PixelRGBA& p) {
	std::unique_lock lock(lockstate);
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
		return -1;
	}
	return 0; // Success
}



/// Currently can only load 8bit image due to limitation of the FileReader Library
int ImageEntry::LoadImage() {

	std::unique_lock lock(lockstate);

	if (IsLoaded()) return 0; // Already loaded

	try {
		FileReader::ReadImage(path, imageData);

		width = imageData.width;
		height = imageData.height;
		channels = imageData.channels;

		status = CompressionStatus::DECOMPRESSED; // Set the status to loaded
		return 0; // Success

	}
	catch (const std::exception&) {
		return -1; // Error loading image
	}
}
int ImageEntry::UnloadImage() {
	std::unique_lock lock(lockstate);

	if (!IsFree()) return -2; //Image is still in use

	status = CompressionStatus::NOT_LOADED;

	imageData.Clear();
	std::vector<PixelRGBA>().swap(imageData.data);

	imageDataCompressed.clear();
	std::vector<uint8_t>().swap(imageDataCompressed);

	width = 0;
	height = 0;
	channels = 0;

	return 0;
}


int ImageEntry::CompressImageData() {
	std::unique_lock lock(lockstate);
	if (status != CompressionStatus::DECOMPRESSED) return -2;

	QOICompress(std::span<uint8_t>(reinterpret_cast<uint8_t*>(imageData.data.data()), width * height * sizeof(PixelRGBA)), imageDataCompressed, width, height, channels);

	status = CompressionStatus::COMPRESSED;
	imageData.Clear();
	std::vector<PixelRGBA>().swap(imageData.data);

	return 0;
}

int ImageEntry::DecompressImageData() {
	std::unique_lock lock(lockstate);
	if (status != CompressionStatus::COMPRESSED) return -2;

	QOIDecompress(std::span<uint8_t>(imageDataCompressed.data(), imageDataCompressed.size()), imageData.data, width, height);

	status = CompressionStatus::DECOMPRESSED;
	imageDataCompressed.clear();
	std::vector<uint8_t>().swap(imageDataCompressed);

	return 0;
}



#pragma endregion



#pragma region Image Manager


std::shared_ptr<ImageEntry> ImageEntry::AcquireRead() const {
	assert(IsDecompressed());
	return std::const_pointer_cast<ImageEntry>(shared_from_this());
}

bool ImageEntry::IsLoaded() const { return status != CompressionStatus::NOT_LOADED; }
bool ImageEntry::IsDecompressed() const { return status == CompressionStatus::DECOMPRESSED; }
bool ImageEntry::IsFree() const { return shared_from_this().use_count() == 1; }
std::string ImageEntry::GetFilePath() const { return path; }
int ImageEntry::GetWidth() const { return width; }
int ImageEntry::GetHeight() const { return height; }
int ImageEntry::GetChannels() const { return channels; }

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


#pragma region Image Renderer

ImageRenderer::ImageRenderer(std::shared_ptr<ImageEntry> Image) : source(Image), textureID(0), textureLoaded(false), width(0), height(0) {};

void ImageRenderer::LoadGPU() {
	width = source->GetWidth();
	height = source->GetHeight();
	LoadImage_s(textureID, reinterpret_cast<void*>(const_cast<PixelRGBA*>(source->ReadImageData().data())), width, height, source->GetChannels());
	textureLoaded = true;
}

void ImageRenderer::UnloadGPU() {
	width = 0;
	height = 0;
	UnloadImage_s(textureID);
	textureLoaded = false;
}

int ImageRenderer::DisplayImage() {
	if (textureLoaded) {
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::ImageWithBg(textureID, dimension, uv_min, uv_max, bgColor);

		if (ImGui::BeginItemTooltip())
		{

			float region_x = io.MousePos.x - pos.x - widgetDim.x * 0.5f; //Top left XY Coordinates of the region
			float region_y = io.MousePos.y - pos.y - widgetDim.y * 0.5f;
			float zoom = 8.0f;

			//Clamp the region to be within the texture bounds
			if (region_x < 0.0f) { region_x = 0.0f; }
			else if (region_x > dimension.x - widgetDim.x) { region_x = dimension.x - widgetDim.x; }
			if (region_y < 0.0f) { region_y = 0.0f; }
			else if (region_y > dimension.y - widgetDim.y) { region_y = dimension.y - widgetDim.y; }

			// Display the region coordinates and size
			ImGui::Text("Image coord (%.2f, %.2f)", io.MousePos.x - pos.x, io.MousePos.y - pos.y);
			ImGui::SameLine();
			PixelRGBA P;
			if (source->GetPixel(io.MousePos.x - pos.x, io.MousePos.y - pos.y, P) == 0) {
				ImGui::Text("RGBA Val (%.2f, %.2f, %.2f, %.2f)", P.r, P.g, P.b, P.a);
			}
			ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
			ImGui::Text("Max: (%.2f, %.2f)", region_x + widgetDim.x, region_y + widgetDim.y);


			ImVec2 uv0 = ImVec2((region_x) / dimension.x, (region_y) / dimension.y);
			ImVec2 uv1 = ImVec2((region_x + widgetDim.x) / dimension.x, (region_y + widgetDim.y) / dimension.y);
			ImGui::ImageWithBg(textureID, ImVec2(widgetDim.x * zoom, widgetDim.y * zoom), uv0, uv1, bgColor);
			ImGui::EndTooltip();
		}
		return 0;
	}
	else return -1;
}

#pragma endregion

