#include "string"
#include "ImageContainer.h"
#include "FileReader.h"
#include "QOIFormat.h"


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


#pragma endregion
