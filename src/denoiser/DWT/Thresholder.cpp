#include "denoiser.h"

void Denoiser::VisuShrink::InplaceSoftThreshold(std::span<double> array) {
	std::vector<double> copyForMedian = std::vector<double>(array.size());

	//Copies the array into a temporary buffer
	//Since quickselect is inplace and modifies the array
	memmove(&copyForMedian[0], &array[0], array.size() * sizeof(double));


}