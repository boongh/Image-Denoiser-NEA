#pragma once
#include <string>
#include <vector>

/// <summary>
/// Container class for pixel data in an image.
/// Supports up to 32 bit RGBA color channels.
/// </summary>
class pixel {
	public:
	int r, g, b, a; // RGBA color channels
	pixel(int red, int green, int blue, int alpha = 255) : r(red), g(green), b(blue), a(255){};
	// Additional methods for pixel manipulation can be added here
};


/// This class is used to manage image data, including loading, decompressing, and manipulating pixel values.
/// It provides methods to get and set pixel values, as well as to check the status of the image (loaded, decompressed, etc.).
/// /// The image data is stored in a vector of pixel structures, which contain the red, green, blue, and alpha components of each pixel.
/// /// The class also includes methods to check if the image is loaded and decompressed, and to retrieve the width, height, and number of channels of the image.
class ImageEntry {
private:
	/// Metadata for the image entry status.
	bool isLoaded; // Flag to indicate if the image is loaded in memory from lazy loading
	bool isDecompressed; // Flag to indicate if the image is decompressed in memory 
	
	//Metadata for the image
	int width;
	int height;
	int channels; // Number of color channels (e.g., 1 for grayscale, 3 for RGB, 4 for RGBA)

	/// Path is stored for lazy loading of the image data.
	std::string path; //file path of the image entry

	/// container for compressing and decompressing image data to save memory.
	std::vector<pixel> imageDataCompressed; // Pointer to the compressed image data in memory
	std::vector<pixel> imageData; // Pointer to the actual image data in memory

public:

	ImageEntry(const std::string& filePath, int imgWidth, int imgHeight, int imgChannels)
		: path(filePath), width(imgWidth), height(imgHeight), channels(imgChannels),
		isLoaded(false), isDecompressed(false) {
	}

	/// <summary>
	/// Get the raw image data
	/// </summary>
	/// <returns></returns>
	std::vector<pixel>& GetImageData();

	/// <summary>
	/// Sets the color values of a specific pixel in the image.
	/// </summary>
	/// <param name="x">The x-coordinate of the pixel to set.</param>
	/// <param name="y">The y-coordinate of the pixel to set.</param>
	/// <param name="r">The red component value to assign to the pixel.</param>
	/// <param name="g">The green component value to assign to the pixel.</param>
	/// <param name="b">The blue component value to assign to the pixel.</param>
	/// <param name="a">The alpha (transparency) component value to assign to the pixel.</param>
	/// <returns>Returns 0 on success, -1 if the coordinates are out of bounds, or -2 if the image is not fully loaded or decompressed.</returns>
	int SetPixel(int x, int y, int r, int g, int b, int a = 255);


	/// <summary>
	/// Sets the value of a pixel at the specified (x, y) coordinates in the image.
	/// </summary>
	/// <param name="x">The x-coordinate of the pixel to set.</param>
	/// <param name="y">The y-coordinate of the pixel to set.</param>
	/// <param name="p">The pixel value in a container class to assign at the specified coordinates.</param>
	/// <returns>Returns 0 on success, -1 if the coordinates are out of bounds, or -2 if the image is not fully loaded or decompressed.</returns>
	int SetPixel(int x, int y, const pixel& p);

	/// <summary>
	/// Load image data from the file path into memory.
	/// </summary>
	/// <returns>true if the image was successfully loaded; false otherwise</returns>
	int LoadImage(); // Load the image data from the file path into memory

	/// <summary>
	/// Releases or unloads the currently loaded image from memory.
	/// </summary>
	/// <returns>true if the image was successfully unloaded; false otherwise.</returns>
	int  UnloadImage(); // Unload the image data from memory

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


	//Getters for metadata
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	int GetChannels() const { return channels; }

	// Additional methods for processing or accessing image data can be added here
};

class ImageManager {

};