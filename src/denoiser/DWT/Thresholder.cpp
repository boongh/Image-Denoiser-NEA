#include "denoiser.h"
#include <iostream>

void Denoiser::VisuShrink::SoftThreshold(std::span<float> array) {
	//No need to be perfect median, approx median.
	float median = MathsUtils::QuickSelect<float>(
		array, 
		0, 
		(array.size() - 1), 
		(array.size() / 2)); 

	int M = array.size();
	float standardDeviation = std::abs(median / 0.06745f);
	float threshold = standardDeviation * std::sqrt(2.0f * std::log(static_cast<float>(M)));

	MathsUtils::SoftThreshold(array, threshold);

	std::cout << threshold << " Threshold" << std::endl;
}