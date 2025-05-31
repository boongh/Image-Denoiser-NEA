#pragma once

#include <glad/glad.h>
#include <iostream>
#include "ImGuiimageloader.h"

bool LoadImage(unsigned int& textureID, void* data, unsigned int width, unsigned int height, unsigned int channels) {

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
