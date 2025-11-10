#include "denoiser.h"
#include <iostream>
#include <algorithm>

 void Denoiser::VisuShrink::SoftThreshold(std::span<float> array) {

	std::vector<float> copyArray(array.begin(), array.end());
	std::sort(copyArray.begin(), copyArray.end());
	float median2 = copyArray[(array.size() - 1) / 2];

	int M = array.size();
	float standardDeviation = std::abs(median2 / 0.06745f);
	float threshold = standardDeviation * std::sqrt(2.0f * std::log(static_cast<float>(M)));

	MathsUtils::SoftThreshold(array, threshold);
}