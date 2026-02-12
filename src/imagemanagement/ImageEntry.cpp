#include "string"
#include "ImageContainer.h"
#include "FileReader.h"
#include "QOIFormat.h"

///Implementation of ImageContainer.h

//<---Image Entry class Implementation--->
#pragma region Image Entry


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

std::tuple<std::unique_lock<std::shared_mutex>, std::span<PixelRGBA>> ImageEntry::ReadWriteImageData()
{
	return std::tuple<std::unique_lock<std::shared_mutex>, std::span<PixelRGBA>>{
		std::unique_lock<std::shared_mutex>(lockstate),
			std::span<PixelRGBA>(imageData.data.data(), imageData.data.size())
	};
}

std::tuple<std::shared_lock<std::shared_mutex>, std::shared_ptr<const RGBAImageI>> ImageEntry::RGBAIRead()
{
	//Returns null ptr if an unexpected exception happen
	//Should never return, but can, so check.
	std::shared_ptr<const RGBAImageI> ptr = nullptr;

	if (status == CompressionStatus::DECOMPRESSED) {
		ptr = std::make_shared<const RGBAImageI>(imageData);
	}
	return std::tuple<std::shared_lock<std::shared_mutex>, std::shared_ptr<const RGBAImageI>>{
		std::shared_lock<std::shared_mutex>(lockstate),
			ptr
	};
}

std::tuple<std::unique_lock<std::shared_mutex>, std::shared_ptr<RGBAImageI>> ImageEntry::RGBAIReadWrite()
{
	return std::tuple<std::unique_lock<std::shared_mutex>, std::shared_ptr<RGBAImageI>>{
		std::unique_lock<std::shared_mutex>(lockstate),
			std::make_shared<RGBAImageI>(imageData)
	};
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

	if (CheckBound(x, y) != 0) {
		return -1; // Out of bounds
	}
	else if (!IsDecompressed()) {
		return -2; // Image not fully in memory
	}
	else if (!IsFree()) return -2;

	imageData[(y * width + x)] = PixelRGBA(r, g, b, a);

	return 0; // Success
}


int ImageEntry::GetPixel(int x, int y, PixelRGBA& p) const {
	std::shared_lock lock(lockstate);
	if (!IsDecompressed()) {
		return -1; // Image not fully in memory
	}

	p = imageData.data.at(x + width * y);
	return 0;
}


int ImageEntry::SetPixel(int x, int y, const PixelRGBA& p) {
	std::unique_lock lock(lockstate);
	
	if (CheckBound(x, y) != 0) {
		return -1; // Out of bounds
	}
	else if (!IsDecompressed()) {
		return -2; // Image not fully in memory
	}

	imageData[(y * width + x)] = p;
	return 0; // Success
}



/// Currently can only load 8bit image due to limitation of the FileReader Library
int ImageEntry::LoadImage() {
	std::unique_lock lock(lockstate);

	if (IsLoaded()) return 0; // Already loaded

	if (FileReader::ReadImage(path.string(), imageData)) {
		width = imageData.width;
		height = imageData.height;
		channels = imageData.channels;

		status = CompressionStatus::DECOMPRESSED; // Set the status to loaded
		return 0; // Success
	}
	return -1; // Error loading image
}

int ImageEntry::UnloadImage() {
	std::unique_lock lock(lockstate);

	if (!IsFree()) return -2; //Image is still in use

	status = CompressionStatus::NOT_LOADED;

	imageData.Clear();
	imageData.data.resize(0);
	imageData.data.shrink_to_fit();

	imageDataCompressed.clear();
	imageDataCompressed.resize(0);
	imageDataCompressed.shrink_to_fit();

	width = 0;
	height = 0;
	channels = 0;

	return 0;
}


int ImageEntry::CompressImageData() {
	std::unique_lock lock(lockstate);


	//Already compressed
	if (status == CompressionStatus::COMPRESSED) return 0;


	//Not even loaded in
	if (status != CompressionStatus::DECOMPRESSED) return -2;


	if (QOICompress(std::span<uint8_t>(reinterpret_cast<uint8_t*>(imageData.data.data()), width * height * sizeof(PixelRGBA)), imageDataCompressed, width, height, channels) == -1) return -1;

	status = CompressionStatus::COMPRESSED;
	imageData.data.resize(0);
	imageData.data.shrink_to_fit();

	return 0;
}

int ImageEntry::DecompressImageData() {
	std::unique_lock lock(lockstate);

	//Image already decompressed
	if (status == CompressionStatus::DECOMPRESSED) return 0;

	//Image not even loaded in
	if (status != CompressionStatus::COMPRESSED) return -2;

	if (status == CompressionStatus::COMPRESSED) {
		QOIDecompress(std::span<uint8_t>(imageDataCompressed.data(), imageDataCompressed.size()), imageData.data, width, height);

		status = CompressionStatus::DECOMPRESSED;
		imageDataCompressed.clear();

		imageData.width = width;
		imageData.height = height;
		imageData.channels = channels;

		imageDataCompressed.resize(0);
		imageDataCompressed.shrink_to_fit();

	}
	return 0;
}

int ImageEntry::TryCompressImageData() {
	std::unique_lock lock(lockstate, std::defer_lock);

	if (!lock.try_lock())
		return 1;


	//Already compressed
	if (status == CompressionStatus::COMPRESSED) return 0;

	//Not even loaded in
	if (status != CompressionStatus::DECOMPRESSED) return -2;

	//Someone else still hold the image, cannot do destructive operations
	if (IsFree() != 0) return -3;

	if (QOICompress(std::span<uint8_t>(reinterpret_cast<uint8_t*>(imageData.data.data()), width * height * sizeof(PixelRGBA)), imageDataCompressed, width, height, channels) == -1) return -1;

	status = CompressionStatus::COMPRESSED;
	imageData.data.resize(0);
	imageData.data.shrink_to_fit();

	return 0;

}

int ImageEntry::TryDecompressImageData()
{
	std::unique_lock lock(lockstate, std::defer_lock);

	if (!lock.try_lock())
		return 1;

	//Image already decompressed
	if (status == CompressionStatus::DECOMPRESSED) return 0;

	//Image not even loaded in
	if (status != CompressionStatus::COMPRESSED) return -2;

	if (status == CompressionStatus::COMPRESSED) {
		QOIDecompress(std::span<uint8_t>(imageDataCompressed.data(), imageDataCompressed.size()), imageData.data, width, height);

		status = CompressionStatus::DECOMPRESSED;
		imageDataCompressed.clear();

		imageData.width = width;
		imageData.height = height;
		imageData.channels = channels;

		imageDataCompressed.resize(0);
		imageDataCompressed.shrink_to_fit();

	}

	return 0;
}

std::shared_ptr<ImageEntry> ImageEntry::AcquireRead() const {
	return std::const_pointer_cast<ImageEntry>(shared_from_this());
}

bool ImageEntry::IsLoaded() const { return status != CompressionStatus::NOT_LOADED; }
bool ImageEntry::IsDecompressed() const { return status == CompressionStatus::DECOMPRESSED; }
bool ImageEntry::IsFree() const { return shared_from_this().use_count() == 1; }

std::filesystem::path ImageEntry::GetSourcePath() const { return path; }
std::filesystem::path ImageEntry::GetFileName() const { return path.filename(); }

int ImageEntry::GetWidth() const {
	if (IsDecompressed() || IsLoaded()) { // Ensure the image is loaded or decompressed before accessing width
		return width;
	}
	return -1;
}
int ImageEntry::GetHeight() const {
	if (IsDecompressed() || IsLoaded()) { // Ensure the image is loaded or decompressed before accessing width
		return height;
	}
	return -1;
}
int ImageEntry::GetChannels() const {
	if (IsDecompressed() || IsLoaded()) { // Ensure the image is loaded or decompressed before accessing width
		return channels;
	}
	return -1;
}

ImageEntry::CompressionStatus ImageEntry::GetStatus() const { return status; }


#pragma endregion