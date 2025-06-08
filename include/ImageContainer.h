#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <span>
#include <FileFormats.h>
#include <imgui.h>

/// This class is used to manage image data, including loading, decompressing, and manipulating pixel values.
/// It provides methods to get and set pixel values, as well as to check the status of the image (loaded, decompressed, etc.).
/// /// The image data is stored in a vector of pixel structures, which contain the red, green, blue, and alpha components of each pixel.
/// /// The class also includes methods to check if the image is loaded and decompressed, and to retrieve the width, height, and number of channels of the image.
struct ImageEntry {
	enum class CompressionStatus; // Forward declaration for compression status enum

	/// <summary>
	/// Thread safety lock to ensure sequential access to the image entry.
	/// </summary>
	mutable std::mutex lockstate; // Mutex to protect the state of the image entry

	/// <summary>
	/// Status of the image entry.
	/// </summary>
	CompressionStatus status = CompressionStatus::UNLOADED; // Status of the image data compression


	/// <summary>
	/// Use count to check safety of compressing image data
	/// </summary>
	mutable std::atomic<int> useCount = 0; // Atomic counter for tracking the number of users of this image entry


	//Metadata for the image
	int width;
	int height;
	int channels; // Number of color channels (e.g., 1 for grayscale, 3 for RGB, 4 for RGBA)

	/// Path is stored for lazy loading of the image data.
	std::string path; //file path of the image entry

	/// container for compressing and decompressing image data to save memory.
	std::vector<uint8_t> imageDataCompressed; // Vector storing compressed image data
	RGBAImageI imageData;


	/// <summary>
	/// TextureID used for OpenGL rendering
	/// </summary>
	unsigned int textureID;
	bool TextureLoaded;


	/// <summary>
	/// Release the image for read. No locking.
	/// </summary>
	void ReleaseRead() const;

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

	/// <summary>
	/// RAII class to handle automatic acquire and release of resource when running out of scope
	/// </summary>
	struct ImageAccess {
		~ImageAccess();
	public:
		const ImageEntry& source;
		void Release();
		ImageAccess(ImageEntry& entrySource);
	};

	/// <summary>
	/// Initialization of an entry
	/// Requires manual LoadImage() to fully access it
	/// </summary>
	/// <param name="filePath"></param>
	ImageEntry(const std::string& filePath);


	/// <summary>
	/// Gets read access to the entire image data
	/// </summary>
	/// <returns></returns>
	std::span<const PixelRGBA> ReadImageData() const;


	/// <summary>
	/// WIP
	/// Returns a vector of a span of a rectangular region of the image
	/// </summary>
	/// <returns></returns>
	ImageAccess ReadSpan(int tlx, int tly, int brx, int bry) const;

	/// <summary>
	/// Check if the pixel is a valid coordinate
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <returns></returns>
	bool CheckBound(int x, int y) const;

	/// <summary>
	/// Writes image data
	/// </summary>
	/// <returns></returns>
	int WriteSpan(int tlx, int tly, std::vector<std::span<PixelRGBA>> src);


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
	/// 
	/// </summary>
	/// <returns>Texture ID of the image if loaded</returns>
	unsigned int GetTextureID() const;


	/// <summary>
	/// Load image data from the file path into memory.
	/// </summary>
	/// <returns>0 for success, -1 for error loading image, -2 if image is in use (for whatever reason)</returns>
	int LoadImage(); // Load the image data from the file path into memory


	/// <summary>
	/// Load the image into an opengl texture
	/// </summary>
	/// <returns></returns>
	int LoadTexture();


	/// <summary>
	/// Releases or unloads the currently loaded image from memory.
	/// </summary>
	/// <returns>true if the image was successfully unloaded; false otherwise.</returns>
	int UnloadImage(); // Unload the image data from memory


	/// <summary>
	/// Unload texture from opengl
	/// </summary>
	/// <returns></returns>
	int UnloadTexture();

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
	ImageAccess AcquireRead() const;


	//Get status of image
	bool IsLoaded() const; // Check if the image is loaded
	bool IsDecompressed() const; // Check if the image is decompressed
	bool IsFree() const; // Check if the image is free (not in use)


	//<--Getters for metadata-->
	std::string GetFilePath() const;

	int GetWidth() const;
	int GetHeight() const;
	int GetChannels() const;

	///<---Disallow copy and moving for now for safety--->
	///WIP will allow later when processing the same image twice is allowed

	//Disallow copy
	ImageEntry(const ImageEntry&) = delete;
	ImageEntry& operator =(const ImageEntry&) = delete;

	//disallow move
	ImageEntry(ImageEntry&& other) = delete;
	ImageEntry& operator=(ImageEntry&& other) = delete;

	
	// Additional methods for processing or accessing image data can be added here
};



struct ImageManager {
	//WIP

	std::vector<ImageEntry*> imageEntries;

public:
	int GetImageCount();

	int ImportFromFile(std::string path);
	int LazyLoadImage(int index);

	void Compress(int index);
	void Decompress(int index);

	void LoadGPU(int index);
	void UnloadGPU(int index);


	void DisplayImage(int index);

	ImageEntry::ImageAccess ReadImage(int id);
	std::string GetName(int id) const;
	ImVec2 GetDim(int id) const;

};