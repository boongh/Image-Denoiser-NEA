#pragma once
#include <string>
#include <vector>
#include <shared_mutex>
#include <span>
#include <FileFormats.h>
#include <imgui.h>
#include <unordered_map>
#include <set>
#include <filesystem>

/// This class is used to manage image data, including loading, decompressing, and manipulating pixel values.
/// /// The Image Data is stored as RGBAImageI from FileReader library
class ImageEntry : public std::enable_shared_from_this<ImageEntry> {
public:
	enum class CompressionStatus; // Forward declaration for compression status enum
private:
	

	/// <summary>
	/// Status of the image entry.
	/// </summary>
	CompressionStatus status = CompressionStatus::NOT_LOADED;

	// Can't read when someone is writing
	// Can't write when someone is reading
	// Can't write when someone is writing
	// 
	// Write permission
	// Read count
	// 
	// Read read
	// Read write
	// Write write
	// Write read
	// 
	// Can't read when someone is writing
	// Can read when someone is 
	// Read write locks mutex

	// More than one person can read
	// 
	// 
	// Only one person can write

	mutable std::shared_mutex lockstate;

	//Metadata for the image
	int width;
	int height;
	int channels;

	/// Path is stored for lazy loading of the image data.
	std::filesystem::path path; //file path of the image entry

	/// container for compressing and decompressing image data to save memory.
	std::vector<uint8_t> imageDataCompressed;
	RGBAImageI imageData;


public:

	/// <summary>
	/// Mark the status of the image data compression.
	/// Unloaded: Image data is not compressed and is in memory.
	/// Compressed: Image data is compressed and stored in memory.
	/// Decompressed: Image data is decompressed and ready for processing.
	/// </summary>
	enum class CompressionStatus {
		NOT_LOADED,
		COMPRESSED,
		DECOMPRESSED,
	};

	/// <summary>
	/// Initialization of an entry
	/// Requires manual LoadImage() to fully access it
	/// </summary>
	/// <param name="filePath"></param>
	ImageEntry(const std::string& filePath);
	ImageEntry(std::span<PixelRGBA> src, unsigned int width, unsigned int height, std::string name);

	/// <summary>
	/// Gets read access to the entire image data
	/// </summary>
	/// <returns></returns>
	std::span<const PixelRGBA> ReadImageData() const;
	std::tuple<std::unique_lock<std::shared_mutex>, std::span<PixelRGBA>> ReadWriteImageData();
	

	/// <summary>
	/// Direct access to the RGBAImageI object with write permission
	/// Rarely used since most functions deal with spans of PixelRGBA
	/// </summary>
	/// <returns></returns>
	std::tuple<std::shared_lock<std::shared_mutex>, std::shared_ptr<const RGBAImageI>> RGBAIRead();
	std::tuple<std::unique_lock<std::shared_mutex>, std::shared_ptr<RGBAImageI>> RGBAIReadWrite();


	/// <summary>
	/// Check if the pixel is a valid coordinate
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <returns></returns>
	bool CheckBound(size_t x, size_t y) const;

	/// <summary>
	/// Writes image data
	/// </summary>
	/// <returns></returns>
	int WriteSpan(size_t tlx, size_t tly, std::vector<std::span<PixelRGBA>> src);


	/// <summary>
	/// Sets the color values of a specific pixel in the image.
	/// </summary>
	/// <param name="x">X coordinate from top left</param>
	/// <param name="y">Y coordinate from top left</param>
	/// <param name="r">The red component value to assign to the pixel.</param>
	/// <param name="g">The green component value to assign to the pixel.</param>
	/// <param name="b">The blue component value to assign to the pixel.</param>
	/// <param name="a">The alpha (transparency) component value to assign to the pixel.</param>
	/// <returns>Returns 0 on success, -1 if the coordinates are out of bounds, or -2 if the image is not fully loaded.</returns>
	int SetPixel(int x, int y, int r, int g, int b, int a = 255);


	/// <summary>
	/// Get a copy of the pixel data of a specific pixel in the image.
	/// </summary>
	/// <param name="x">X coordinate from top left</param>
	/// <param name="y">Y coordinate from top left</param>
	/// <param name="p">Pixel information</param>
	/// <returns>Return 0 on success, -1 if the coordinates are out of bound, or -2 if the image is not fully loaded memory</returns>
	int GetPixel(int x, int y, PixelRGBA& p) const;


	/// <summary>
	/// Sets the value of a pixel at the specified (x, y) coordinates in the image.
	/// </summary>
	/// <param name="x">The x-coordinate of the pixel to set.</param>
	/// <param name="y">The y-coordinate of the pixel to set.</param>
	/// <param name="p">The pixel value in a container class to assign at the specified coordinates.</param>
	/// <returns>Returns 0 on success, -1 if the coordinates are out of bounds, or -2 if the image is not fully loaded or decompressed.</returns>
	int SetPixel(int x, int y, const PixelRGBA& p);


	/// <summary>
	/// Load image data from the file path into memory.
	/// </summary>
	/// <returns>0 for success, -1 for error loading image, -2 if image is in use (for whatever reason)</returns>
	int LoadImage();

	/// <summary>
	/// Releases or unloads the currently loaded image from memory.
	/// </summary>
	/// <returns>true if the image was successfully unloaded; false otherwise.</returns>
	int UnloadImage();

	int CompressImageData();
	int DecompressImageData();


	//<--Thread-safety functions to manage the use count of the image entry-->

	/// <summary>
	/// Acquires the image for read. No locking.
	/// </summary>
	std::shared_ptr<ImageEntry> AcquireRead() const;

	//Get status of image
	bool IsLoaded() const;
	bool IsDecompressed() const;

	//No one else holds the shared_ptr
	bool IsFree() const;


	//<--Getters for metadata-->
	std::string GetFilePathString() const;
	std::filesystem::path GetFilePath_path() const;
	std::filesystem::path GetFileName() const;

	int GetWidth() const;
	int GetHeight() const;
	int GetChannels() const;

	CompressionStatus GetStatus() const;

	///<---Disallow copy and moving for now for safety--->
	///WIP will allow later when processing the same image twice is allowed

	//Disallow copy
	ImageEntry(ImageEntry&) = delete;
	ImageEntry& operator =(const ImageEntry&) = delete;

	//disallow move
	ImageEntry(ImageEntry&& other) = delete;
	ImageEntry& operator=(ImageEntry&& other) = delete;

	
	// Additional methods for processing or accessing image data can be added here
};

class ImageRenderer {
public:
	ImageRenderer(std::shared_ptr<ImageEntry> Image);
	~ImageRenderer();

	void LoadGPU();
	void UnloadGPU();

	int DisplayImage(ImVec2 widgetDimension, ImVec4 bgColor);

private:
	
	unsigned int width;
	unsigned int height;

	const std::shared_ptr<ImageEntry> source;
	bool textureLoaded = false;
	unsigned int textureID;
};

class ImageManager {

public:
	size_t GetImageCount();

	std::vector<std::shared_ptr<ImageEntry>>::iterator begin();
	std::vector<std::shared_ptr<ImageEntry>>::const_iterator begin() const;
	std::vector<std::shared_ptr<ImageEntry>>::const_iterator cbegin() const;

	std::vector<std::shared_ptr<ImageEntry>>::iterator end();
	std::vector<std::shared_ptr<ImageEntry>>::const_iterator end() const;
	std::vector<std::shared_ptr<ImageEntry>>::const_iterator cend() const;

	int ImportFromFile(std::string path);
	int ImportFromSpan(std::span<PixelRGBA> src, unsigned int width, unsigned int height, std::string name);

	int LazyLoadImage(int index);
	int LazyLoadImage(std::shared_ptr<ImageEntry> imageEntry);

	int UnloadImage(int index);
	int UnloadImage(std::set<std::shared_ptr<ImageEntry>> scheduledDeletion);

	ImageEntry::CompressionStatus GetStatus(int index) const;

	void Compress(int index);
	void Decompress(int index);

	std::shared_ptr<ImageRenderer>	CreateRenderer(int index);
	std::shared_ptr<ImageRenderer> CreateRenderer(std::shared_ptr<ImageEntry> item);

	std::shared_ptr<ImageRenderer> GetRenderer(int index);
	std::shared_ptr<ImageRenderer> GetRenderer(std::shared_ptr<ImageEntry> imageEntry);
	
	void DestroyRenderer(int index);
	void DestroyRenderer(std::shared_ptr<ImageEntry> imageEntry);

	std::shared_ptr<ImageEntry> GetImage(int id);

	std::string GetPath(int id) const;
	std::filesystem::path GetPath_path(int id) const;
	ImVec2 GetDim(int id) const;

	void RefreshRenderer(std::shared_ptr<ImageEntry> imageEntry);

private:
	//WIP

	std::vector<std::shared_ptr<ImageEntry>> imageEntries;
	std::unordered_map<std::shared_ptr<ImageEntry>, std::shared_ptr<ImageRenderer>> imageRenderers;

};