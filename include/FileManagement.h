#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <FileFormats.h>
#include <functional>
#include <regex>

typedef std::function<std::string(std::smatch)> ValidFormatter;

enum ImageFormat {
	FORMAT_JPEG,
	FORMAT_PNG,
	FORMAT_BMP,
	FORMAT_TGA,
	FORMAT_QOI,
};

extern const char* formatfilter[];

extern const int formatfiltercount;

int SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths);
std::string NormalizePath(const char* rawPath); 


/// <summary>
/// Open a file multi selection dialogue
/// </summary>
/// <param name="formatfilter"></param>
/// <param name="filtercount"></param>
/// <param name="paths">reference to store the paths selected</param>
/// <returns>Success code
/// 0 - Success
/// Others - Failure
/// </returns>
int FileSelection(
	const char* title,
	const char* defaultPath,
	const char* const* formatfilter,
	int filtercount,
	const char* singlefilterdesc,
	int allowmultiselect,
	std::vector<std::string>& paths);

int SaveImages(std::vector<std::tuple<std::filesystem::path, const RGBAImageI>> list, ImageFormat format);
std::filesystem::path FormatPath(std::unordered_map<std::string, ValidFormatter> formats, std::filesystem::path path);
int PrepFilePath(std::filesystem::path& path);
std::filesystem::path ExtendsFileName(std::filesystem::path file, std::string extends);
std::string RegexReplacement(
	const std::string& input,
	const std::regex regexFormats,
	const ValidFormatter formatter
);