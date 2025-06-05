#pragma once
#include <string>
#include <vector>

class ImageContainer {
private:
	unsigned int textureID; // OpenGL texture ID
	void* data; // Image data pointer
	unsigned int width; // Image width
	unsigned int height; // Image height
	unsigned int channels; // Number of channels in the image

public:

	ImageContainer() : textureID(0), data(nullptr), width(0), height(0), channels(0) {}

	ImageContainer(unsigned int texID, void* imgData, unsigned int imgWidth, unsigned int imgHeight, unsigned int imgChannels)
		: textureID(texID), data(imgData), width(imgWidth), height(imgHeight), channels(imgChannels) {
	}

	~ImageContainer() { clear(); }

	void clear() {
		if (data) {
			delete[] static_cast<unsigned char*>(data); // Assuming data is allocated with new[]
			data = nullptr;
		}
		textureID = 0;
		width = 0;
		height = 0;
		channels = 0;
	}
};

class ImageCollection {
	public:
	std::vector<unsigned int> textureIDs; // OpenGL texture IDs
	std::vector<void*> data; // Image data pointers
	std::vector<unsigned int> widths; // Image widths
	std::vector<unsigned int> heights; // Image heights
	std::vector<unsigned int> channels; // Number of channels in the images
	void clear();


};

bool LoadImage(unsigned int& textureID, void* data, unsigned int width, unsigned int height, unsigned int channels);

bool LoadMultipleImages(std::vector<unsigned int&>textureID, std::vector<void*>& data, std::vector<unsigned int>& width, std::vector<unsigned int>& height, std::vector<unsigned int>& channels, int count);

std::string normalizePath(const char* rawPath);