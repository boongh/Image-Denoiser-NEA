#include "FileManagement.h"
#include "Application.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "CustomWidget.h"
#include <iostream>
#include "FileFormats.h"
#include "FileWriter.h"
#include <regex>
#include <tinyfiledialogs.h>

namespace fs = std::filesystem;

const char* formatfilter[] = {
	"*.jpg",
	"*.png",
	"*.bmp",
	"*.tga",
	"*.qoi",
};

const int formatfiltercount = 5;


/// <summary>
/// Split paths outputted by tfd into
/// </summary>
/// <param name="multi"></param>
/// <param name="singlepaths"></param>
/// <returns></returns>
int SplitPaths(const std::string& multi, std::vector<std::string>& singlepaths)
{
	std::string normpaths = NormalizePath(multi.c_str());

	std::vector<std::string> paths;
	std::stringstream sstream(normpaths);
	std::string token;
	while (std::getline(sstream, token, '|')) {
		singlepaths.push_back(token);
	}
	return 0;
}


/// <summary>
/// Normalize windows path to UNIX on Windows system and vice versa
/// Since C++ uses UNIX filesystem, but tfd returns Windows path on Windows system
/// </summary>
/// <param name="rawPath">Windows path</param>
/// <returns>UNIX Path</returns>
std::string NormalizePath(const char* rawPath)
{
	std::string path(rawPath);

#if defined (__WIN32__) || defined (_WIN32) || defined (__CYGWIN32__)
	//Windows path
	std::replace(path.begin(), path.end(), '\\', '/');
#endif // WIN_32

	return path;
}

/// <summary>
/// Wrapper around tinyfd that returns a vector of paths instead for easy management
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
	std::vector<std::string>& paths)
{
	try {
		const char* file = tinyfd_openFileDialog(
			title,
			defaultPath,
			filtercount,
			formatfilter,
			singlefilterdesc,
			allowmultiselect
		);

		if (file != nullptr) {

			SplitPaths(NormalizePath(file), paths); // Split the file path into components if needed

			for (int i = 0; i < paths.size(); i++) {
				LogtoAppTerminal(paths[i] + "\n");
			}

			LogtoAppTerminal(std::to_string(paths.size()) + " files selected." + "\n");
		}
		return 0;

	}
	catch (int err) {
		return err;
	}
}

/// <summary>
/// Validate and prepare path by removing spaces and appending (numbers) if file exists
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
int PrepFilePath(fs::path& path)
{
	try {
		namespace fs = fs;

		//Remove space before and after slashes (Invalid spaces)
		std::regex rgx(R"([\s\\]*(\\)[\s\\]*)");
		path = std::regex_replace(path.string(), rgx, "$1");

		if (!path.parent_path().empty()) {
			fs::create_directories(path.parent_path());
		}

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


/// <summary>
/// Formats a string matched with a Regex
/// Formatter takes in a match
/// </summary>
/// <param name="input"></param>
/// <param name="regexFormats"></param>
/// <param name="formatter"></param>
/// <returns></returns>
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


int SaveImages(std::vector<std::tuple<fs::path, const RGBAImageI>> list, ImageFormat format)
{
	try {
		switch (format)
		{
		case FORMAT_JPEG:
			for (auto& [path, image] : list) {
				if (path.empty()) continue;

				path.replace_extension(".jpg");

				PrepFilePath(path);

				FileWriter::WriteJPEG(
					path.string(),
					image,
					90); //Quality fixed at 90 for now
				LogtoAppTerminal("Saved " + path.string() + "\n");
			}
			break;
		case FORMAT_PNG:
			for (auto& [path, image] : list) {
				if (path.empty()) continue;

				path.replace_extension(".png");

				PrepFilePath(path);

				FileWriter::WritePNG(
					path.string(),
					image);
				LogtoAppTerminal("Saved " + path.string() + "\n");
			}
			break;
		case FORMAT_BMP:
			for (auto& [path, image] : list) {
				if (path.empty()) continue;

				path.replace_extension(".bmp");

				PrepFilePath(path);

				FileWriter::WriteBMP(
					path.string(),
					image);
				LogtoAppTerminal("Saved " + path.string() + "\n");
			}
			break;

		case FORMAT_TGA:
			for (auto& [path, image] : list) {
				if (path.empty()) continue;

				path.replace_extension(".tga");

				PrepFilePath(path);

				FileWriter::WriteTGA(
					path.string(),
					image);
				LogtoAppTerminal("Saved " + path.string() + "\n");
			}
			break;
		case FORMAT_QOI:
			for (auto& [path, image] : list) {
				if (path.empty()) continue;

				path.replace_extension(".qoi");

				PrepFilePath(path);

				FileWriter::WriteQOI(
					path.string(),
					image);
				LogtoAppTerminal("Saved " + path.string() + "\n");
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
/// Format path with custom formatters
/// </summary>
/// <param name="regexFormats"></param>
/// <param name="path"></param>
/// <returns></returns>
fs::path FormatPath(std::unordered_map<std::string, ValidFormatter> regexFormats, fs::path path)
{
	fs::path returnPath = path;
	for(auto& [key, func] : regexFormats) {
		returnPath = RegexReplacement(
			returnPath.string(),
			std::regex(key),
			func
		);
	}
	return returnPath;
}

fs::path ExtendsFileName(fs::path file, std::string extends)
{
	fs::path newpath(file.parent_path() / (file.stem().string() + extends));
	newpath.replace_extension(file.extension());
	return newpath;
}
