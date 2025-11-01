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


const char* formatfilter[] = {
	"*.jpg",
	"*.png",
	"*.bmp",
	"*.tga",
	"*.qoi",
};

const int formatfiltercount = 5;

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

/// <summary>
/// Validate and prepare path by removing spaces and appending (number) if file exists
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
int PrepFilePath(std::filesystem::path& path)
{
	try {
		namespace fs = std::filesystem;

		//Remove space before and after slashes (Invalid spaces)
		std::regex rgx(R"([\s\\]*(\\)[\s\\]*)");
		path = std::regex_replace(path.string(), rgx, "$1");

		fs::create_directories(path.parent_path());
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

		return 0;
	}
	catch (int error) {
		return error;
	}
}

std::string RegexReplacement(
	const std::string& input,
	const std::regex regexFormats,
	const ValidFormatter formatter
) {
	std::string result = "";
	std::smatch match;
	auto index = input.cbegin();

	while (std::regex_search(index, input.end(), match, regexFormats)) {
		
		result += match.prefix().str();

		if (formatter) result += formatter(match);
		else result += match.str();

		//Increment start index to just after the previous match
		index += match.str().length() + match.prefix().length();
	}

	//After last match append the rest of the string
	result += std::string(index, input.cend());
	return result;
}

int SaveImages(std::vector<std::tuple<std::filesystem::path, const RGBAImageI>> list, ImageFormat format)
{
	try {
		switch (format)
		{
		case FORMAT_JPEG:
			for (auto& item : list) {
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
			for (auto& item : list) {
				auto path = std::get<0>(item);
				if (path.empty()) continue;

				path.replace_extension(".png");

				PrepFilePath(path);

				FileWriter::WritePNG(
					path.string(),
					std::get<1>(item));
				std::cout << "Saved " << path.string() << "\n";
			}
			break;
		case FORMAT_BMP:
			for (auto& item : list) {
				auto path = std::get<0>(item);
				if (path.empty()) continue;

				path.replace_extension(".bmp");

				PrepFilePath(path);

				FileWriter::WriteBMP(
					path.string(),
					std::get<1>(item));
				std::cout << "Saved " << path.string() << "\n";
			}
			break;

		case FORMAT_TGA:
			for (auto& item : list) {
				auto path = std::get<0>(item);
				if (path.empty()) continue;

				path.replace_extension(".tga");

				PrepFilePath(path);

				FileWriter::WriteTGA(
					path.string(),
					std::get<1>(item));
				std::cout << "Saved " << path.string() << "\n";
			}
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
	catch (int err) {
		return err;
	}
	
}

/// <summary>
/// Format that path with strftime and other custom formatters
/// </summary>
/// <param name="regexFormats"></param>
/// <param name="path"></param>
/// <returns></returns>
std::filesystem::path FormatPath(std::unordered_map<std::string, ValidFormatter> regexFormats, std::filesystem::path path)
{
	std::filesystem::path returnPath = path;
	for(auto& [key, func] : regexFormats) {
		returnPath = RegexReplacement(
			returnPath.string(),
			std::regex(key),
			func
		);
	}
	return returnPath;
}
