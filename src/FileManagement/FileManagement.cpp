#include "FileManagement.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "CustomWidget.h"
#include <iostream>
#include "FileFormats.h"
#include "FileWriter.h"
#include <regex>
#include <string>
#include <cstring>

int SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths)
{
	std::vector<std::string> paths;
	std::stringstream ss(multi);
	std::string token;
	while (std::getline(ss, token, '|')) {
		std::replace(token.begin(), token.end(), '\\', '/'); // optional normalization
		singlepaths.push_back(token);
	}
	return 0;
}

std::string normalizePath(const char* rawPath)
{
	std::string path(rawPath);
	std::replace(path.begin(), path.end(), '\\', '/');
	return path;
}

int FileSelection(const char* const* formatfilter, unsigned int filtercount, std::vector<std::string>& paths)
{
	const char* file = OpenFileDialogue("Select an Image", formatfilter, filtercount);

	if (file != NULL) {
		std::cout << normalizePath(file) << "\n";

		SplitPaths(normalizePath(file), paths); // Split the file path into components if needed

		for (int i = 0; i < paths.size(); i++) {
			std::cout << paths[i] << std::endl;
		}
		std::cout << paths.size() << " files selected." << std::endl;
	}
	return 0;
}

void PrepFilePath(std::filesystem::path& path)
{
	namespace fs = std::filesystem;
	std::filesystem::create_directories(path.parent_path());
	auto filename = path.stem();
	auto extension = path.extension();

	//Append (number) at the end of the file name if file exists
	//Try until u long max
	//Which is basically impossible to reach with a normal computer
	if (fs::exists(path)) {
		for (unsigned long i = 1; i < ULONG_MAX; i++) {
			path.replace_filename(filename.string() + "(" + std::to_string(i) + ")" + extension.string());
			if (!fs::exists(path)) {
				break;
			}
		}
	}
}

int SaveImages(std::vector<std::tuple<std::filesystem::path, const RGBAImageI>> list, ImageFormat format)
{
	namespace fs = std::filesystem;
	switch (format)
	{
	case FORMAT_JPEG:
		for(auto& item : list) {
			auto path = std::get<0>(item);
			if (path.empty()) continue;

			path.replace_extension(".jpg");

			PrepFilePath(path);

			FileWriter::WriteJPEG(
				path.string(),
				std::get<1>(item),
				90); //Quality fixed at 90 for now
			std::cout << "Saved " << path.string() << "\n";
		}
		break;
	case FORMAT_PNG:
		break;
	case FORMAT_QOI:
		for (auto& item : list) {
			auto path = std::get<0>(item);
			if (path.empty()) continue;

			path.replace_extension(".qoi");

			PrepFilePath(path);

			FileWriter::WriteQOI(
				path.string(),
				std::get<1>(item));
			std::cout << "Saved " << path.string() << "\n";
		}
		break;
	default:
		break;
	}
	return 0;
}

/// <summary>
/// Format that path with strftime and other custom formatters
/// </summary>
/// <param name="regexFormats"></param>
/// <param name="path"></param>
/// <returns></returns>
std::filesystem::path FormatPath(std::unordered_map<std::string, validFormatter> regexFormats, std::filesystem::path path)
{
	std::filesystem::path returnPath = path;
	for(auto& [key, func] : regexFormats) {
		//Format time
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);

		char buffer[256];
		strftime(buffer, 256, returnPath.string().c_str(), timeinfo);
		returnPath = std::string(buffer);

		//Other formats
		returnPath = std::regex_replace(returnPath.string(), std::regex(key), func()); // replace all instances of key with return val of the formatter function
	}
	return returnPath;
}
