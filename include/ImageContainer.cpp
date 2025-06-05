///Implementation of ImageContainer.h

#include <FileReader.h>
#include "ImageContainer.h"
#include <fstream>



std::vector<pixel>& ImageEntry::GetImageData(){
	LoadImage();
	DecompressImageData();

	return imageData; // Return the image data vector
}

///< !-- - ImageEntry Class--->
int ImageEntry::SetPixel(int x, int y, int r, int g, int b, int a){
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return -1; // Out of bounds
	}
	else if (!isLoaded || !isDecompressed) {
		return -2; // Image not fully in memory
	}
	int index = (y * width + x) * channels; // Calculate the index for the pixel

	imageData[index].r = r;
	imageData[index].g = g;
	imageData[index].b = b;
	imageData[index].a = a;

	return 0; // Success
}


int ImageEntry::SetPixel(int x, int y, const pixel& p) {
	if (x < 0 || x >= width || y < 0 || y >= height) {
		return -1; // Out of bounds
	}
	else if (!isLoaded || !isDecompressed) {
		return -2; // Image not fully in memory
	}
	int index = (y * width + x); // Calculate the index for the pixel

	imageData[index] = p; // Set the pixel data

	return 0; // Success
}

int ImageEntry::LoadImage()
{
	RGBAImageI imagebuffer;
	FileReader::ReadImage(path, imagebuffer);

	
}

