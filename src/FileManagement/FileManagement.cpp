#include "FileManagement.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "CustomWidget.h"
#include <iostream>
#include "FileFormats.h"
#include "FileWriter.h"

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

int SaveImages(std::vector<std::tuple<std::filesystem::path, std::shared_ptr<RGBAImageI>>> list, ImageFormat format)
{
	switch (format)
	{
	case FORMAT_JPEG:
		for(auto& item : list) {
			auto path = std::get<0>(item);
			if (path.empty()) continue;
			std::filesystem::create_directories(path.parent_path());
			FileWriter::WriteJPEG(
				path.string(),
				*std::get<1>(item).get(),
				90); //Quality fixed at 90 for now
		}
		break;
	case FORMAT_PNG:
		break;
	case FORMAT_QOI:
		for (auto& item : list) {
			auto path = std::get<0>(item);
			if (path.empty()) continue;
			std::filesystem::create_directories(path.parent_path());
			FileWriter::WriteQOI(
				path.string(),
				*std::get<1>(item).get()); //Quality fixed at 90 for now
		}
		break;
	default:
		break;
	}
	return 0;
}
