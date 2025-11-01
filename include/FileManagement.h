#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <FileFormats.h>
#include <functional>
#include <regex>

typedef std::function<std::string(void)> validFormatter;

enum ImageFormat {
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_QOI,
};;

int SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths);
std::string normalizePath(const char* rawPath); 
int FileSelection(const char* const* formatfilter, unsigned int filtercount, std::vector<std::string>& paths);
int SaveImages(std::vector<std::tuple<std::filesystem::path, const RGBAImageI>> list, ImageFormat format);
std::filesystem::path FormatPath(std::unordered_map<std::string, validFormatter> formats, std::filesystem::path path);