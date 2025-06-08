#pragma once
#include <string>
#include <vector>

/// <summary>
/// Load image and create OpenGL texture.
/// Guarantees that only one image is loaded at a time.
/// </summary>
/// <param name="textureID"></param>
/// <param name="data"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="channels"></param>
/// <returns></returns>
bool LoadImage_s(unsigned int& textureID, void* data, unsigned int width, unsigned int height, unsigned int channels);

bool UnloadImage_s(unsigned int& textureID);


/// <summary>
/// Load images and create OpenGL textures.
/// 
/// </summary>
/// <param name="textureID"></param>
/// <param name="data"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="channels"></param>
/// <param name="count"></param>
/// <returns></returns>
bool LoadMultipleImages(std::vector<unsigned int*>&textureID, std::vector<void*>& data, std::vector<unsigned int>& width, std::vector<unsigned int>& height, std::vector<unsigned int>& channels, int count);

bool SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths);



std::string normalizePath(const char* rawPath);