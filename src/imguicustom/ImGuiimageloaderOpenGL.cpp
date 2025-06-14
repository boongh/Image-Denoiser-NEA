#pragma once

#include <glad/glad.h>
#include <iostream>
#include "ImGuiimageloader.h"
#include <algorithm>
#include <sstream>


bool LoadImage_s(unsigned int& textureID, void* data, unsigned int width, unsigned int height, unsigned int channels) {

	try {

		// Load the image data into the texture
		if (data) {
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			std::cerr << "Failed to load image data." << std::endl;
		}
		return true;
	}
	catch (const std::exception& e) {
		// Unbind the texture
		glBindTexture(GL_TEXTURE_2D, 0);
		std::cerr << "Exception occurred while loading image: " << e.what() << std::endl;
		return false;
	}
}

bool UnloadImage_s(unsigned int& textureID) {
	glDeleteTextures(1, &textureID);
	return false;
}

bool SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths)
{
	std::vector<std::string> paths;
	std::stringstream ss(multi);
	std::string token;
	while (std::getline(ss, token, '|')) {
		std::replace(token.begin(), token.end(), '\\', '/'); // optional normalization
		singlepaths.push_back(token);
	}
	return true;
}

bool LoadMultipleImages(std::vector<unsigned int*>& textureID, std::vector<void*>& data, std::vector<unsigned int>& width, std::vector<unsigned int>& height, std::vector<unsigned int>& channels, int count)
{
	//Initialize return vals
	textureID.clear();
	width.clear();
	height.clear();
	channels.clear();




	return true;
}



std::string normalizePath(const char* rawPath)
{
	std::string path(rawPath);
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}
