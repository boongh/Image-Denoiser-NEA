#pragma once
#include <string>
#include <vector>
#include <filesystem>

enum ImageFormat {
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_QOI,
};;

int SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths);
std::string normalizePath(const char* rawPath); 
int FileSelection(const char* const* formatfilter, unsigned int filtercount, std::vector<std::string>& paths);
int SaveImages(std::vector<std::tuple<std::filesystem::path, std::shared_ptr<RGBAImageI>>> list, ImageFormat format);