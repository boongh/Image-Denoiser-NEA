#pragma once
#include <string>
#include <vector>
#include <mutex>


/// <summary>
/// Container class for pixel data in an image.
/// Supports up to 32 bit RGBA color channels.
/// </summary>
class Pixel {
	public:
	int r, g, b, a; // RGBA color channels
	Pixel() : r(0), g(0), b(0), a(255) {}; // Default constructor initializes pixel to transparent black
	Pixel(int red, int green, int blue, int alpha = 255) : r(red), g(green), b(blue), a(alpha){};
	// Additional methods for pixel manipulation can be added here
};

/// This class is used to manage image data, including loading, decompressing, and manipulating pixel values.
/// It provides methods to get and set pixel values, as well as to check the status of the image (loaded, decompressed, etc.).
/// /// The image data is stored in a vector of pixel structures, which contain the red, green, blue, and alpha components of each pixel.
/// /// The class also includes methods to check if the image is loaded and decompressed, and to retrieve the width, height, and number of channels of the image.
class ImageEntry {
private:
	enum class CompressionStatus; // Forward declaration for compression status enum

	/// <summary>
	/// Thread safety lock to ensure sequential access to the image entry.
	/// </summary>
	std::mutex lockstate; // Mutex to protect the state of the image entry

	/// <summary>
	/// Status of the image entry.
	/// </summary>
	CompressionStatus status = CompressionStatus::UNLOADED; // Status of the image data compression


	/// <summary>
	/// Use count to check safety of compressing image data
	/// </summary>
	std::atomic<int> useCount = 0; // Atomic counter for tracking the number of users of this image entry


	//Metadata for the image
	int width;
	int height;
	int channels; // Number of color channels (e.g., 1 for grayscale, 3 for RGB, 4 for RGBA)

	/// Path is stored for lazy loading of the image data.
	std::string path; //file path of the image entry

	/// container for compressing and decompressing image data to save memory.
	std::vector<uint8_t> imageDataCompressed; // Vector storing compressed image data
	std::vector<Pixel> imageData; // Vector storing raw image data as Pixel structures

public:

	/// <summary>
	/// Mark the status of the image data compression.
	/// Unloaded: Image data is not compressed and is in memory.
	/// Compressed: Image data is compressed and stored in memory.
	/// Decompressed: Image data is decompressed and ready for processing.
	/// </summary>
	enum class CompressionStatus {
		UNLOADED, // Image data is not compressed
		COMPRESSED, // Image data is compressed
		DECOMPRESSED, // Image data is decompressed and ready for processing
	};


	ImageEntry(const std::string& filePath)
		: path(filePath), width(0), height(0), channels(0),
		status(CompressionStatus::UNLOADED){
	}

	/// <summary>
	/// Get the raw image data read only.
	/// </summary>
	/// <returns></returns>
	const std::vector<Pixel>& GetImageData() const;

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
	/// Get pixel data of a specific pixel in the image.
	/// </summary>
	/// <param name="x">X coordinate from top left</param>
	/// <param name="y">Y coordinate from top left</param>
	/// <param name="p">Pixel information</param>
	/// <returns>Return 0 on success, -1 if the coordinates are out of bound, or -2 if the image is not fully loaded memory</returns>
	int GetPixel(int x, int y, Pixel& p) const;


	/// <summary>
	/// Sets the value of a pixel at the specified (x, y) coordinates in the image.
	/// </summary>
	/// <param name="x">The x-coordinate of the pixel to set.</param>
	/// <param name="y">The y-coordinate of the pixel to set.</param>
	/// <param name="p">The pixel value in a container class to assign at the specified coordinates.</param>
	/// <returns>Returns 0 on success, -1 if the coordinates are out of bounds, or -2 if the image is not fully loaded or decompressed.</returns>
	int SetPixel(int x, int y, const Pixel& p);

	/// <summary>
	/// Load image data from the file path into memory.
	/// </summary>
	/// <returns>0 for success, -1 for error loading image, -2 if image is in use (for whatever reason)</returns>
	int LoadImage(); // Load the image data from the file path into memory

	/// <summary>
	/// Releases or unloads the currently loaded image from memory.
	/// </summary>
	/// <returns>true if the image was successfully unloaded; false otherwise.</returns>
	int UnloadImage(); // Unload the image data from memory

	/// <summary>
	/// Attempts to compress image data.
	/// </summary>
	/// <returns>true if the image data was successfully compressed; false otherwise.</returns>
	int CompressImageData(); // Compress the image data to save memory

	/// <summary>
	/// Decompresses the image data for processing.
	/// </summary>
	/// <returns>true if the image was successfully decompressed; false otherwise.</returns>
	int DecompressImageData(); // Decompress the image data for processing


	//<--Thread-safety functions to manage the use count of the image entry-->

	/// <summary>
	/// Acquires the image for read. No locking.
	/// </summary>
	void Acquire();

	/// <summary>
	/// Release the image for read. No locking.
	/// </summary>
	void Release();


	bool IsLoaded() const { return status != CompressionStatus::UNLOADED; } // Check if the image is loaded
	bool IsDecompressed() const { return status == CompressionStatus::DECOMPRESSED; } // Check if the image is decompressed
	bool IsFree() const { return useCount.load() == 0; } // Check if the image is free (not in use)


	//Getters for metadata
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	int GetChannels() const { return channels; }

	// Additional methods for processing or accessing image data can be added here
};

class ImageManager {
	//WIP
};