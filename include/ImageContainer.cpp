///Implementation of ImageContainer.h

#include <FileReader.h>
#include "ImageContainer.h"
#include <fstream>
#include <cassert>


const std::vector<Pixel>& ImageEntry::GetImageData() const{
	assert(IsDecompressed());
	return imageData; // Return the image data vector
}

///< !-- - ImageEntry Class--->
int ImageEntry::SetPixel(int x, int y, int r, int g, int b, int a){
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return -1; // Out of bounds
	} else if(!IsDecompressed()) {
		return -2; // Image not fully in memory
	}
	
	int index = (y * width + x); // Calculate the index for the pixel

	imageData[index].r = r;
	imageData[index].g = g;
	imageData[index].b = b;
	imageData[index].a = a;

	return 0; // Success
}

int ImageEntry::GetPixel(int x, int y, Pixel& p) const
{
	return 0;
}


int ImageEntry::SetPixel(int x, int y, const Pixel& p) {
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return -1; // Out of bounds
	}
	else if (!IsDecompressed()) {
		return -2; // Image not fully in memory
	}
	int index = (y * width + x); // Calculate the index for the pixel

	imageData[index] = p; // Set the pixel data

	return 0; // Success
}

int ImageEntry::LoadImage()
{
	std::lock_guard<std::mutex> lock(lockstate); // Ensure thread safety

	// Check if the image is already loaded or in use, then loading again should not be allowed
	if (useCount > 0) return -2; // Image is already in use, cannot load
	if (IsLoaded()) return 0; // Already loaded

	try
	{
		RGBAImageI imagebuffer;
		FileReader::ReadImage(path, imagebuffer);

		width = imagebuffer.width;
		height = imagebuffer.height;
		channels = imagebuffer.channels;

		imageData.resize(width * height); // Resize the vector to hold the image data

		std::memcpy(imageData.data(), imagebuffer.data.data(), imagebuffer.data.size() * channels);
		status = CompressionStatus::DECOMPRESSED; // Set the status to loaded
		return 0; // Success
	}
	catch (const std::exception&)
	{
		return -1; // Error loading image
	}
	
}

void ImageEntry::Acquire() { // Increment the use count when the image is being used
	useCount.fetch_add(1, std::memory_order_relaxed);
}

void ImageEntry::Release() { // Decrement the use count when the image is no longer being used
	useCount.fetch_sub(1, std::memory_order_relaxed);
}

