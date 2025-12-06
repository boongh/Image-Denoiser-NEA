#include "denoiser.h"
#include <iostream>
#include <algorithm>


template <typename T>
int LomutoPartition(std::vector<T>& arr, int l, int r) {
    T x = arr[r];
    int i = l;
    for (int j = l; j <= r - 1; j++) {
        if (arr[j] <= x) {
            std::swap(arr[i], arr[j]);
            i++;
        }
    }
    std::swap(arr[i], arr[r]);
    return i;
}


template <typename T>
T KthSmallest(std::vector<T>& arr, int l, int r, int k) {

    // If k is smaller than the number of elements
    // in the array.
    if (k > 0 && k <= r - l + 1) {

        // Partition the array around the last 
        // element and get the position of the pivot 
        // element in the sorted array.
        int index = LomutoPartition(arr, l, r);

        // If position is the same as k.
        if (index - l == k - 1)
            return arr[index];

        // If position is more, recurse left.
        if (index - l > k - 1)
            return KthSmallest(arr, l, index - 1, k);

        // Else recurse right.
        if(index - l < k - 1)
            return KthSmallest(arr, index + 1, r, k - index + l - 1);
    }

    // If k is more than the number of elements in the array.
    return INT_MAX;
}


void Denoiser::VisuShrink::SoftThreshold(std::span<float> array) {

	std::vector<float> copyArray(array.begin(), array.end());

    float median = KthSmallest(copyArray, 0, copyArray.size() - 1, (copyArray.size() + 1) / 2);

	/*std::sort(copyArray.begin(), copyArray.end());
	float median2 = copyArray[(array.size() - 1) / 2];

    std::cout << "Median : " << median << "\n";
    std::cout << "Median2 : " << median2 << "\n";*/

	int M = array.size();
	float standardDeviation = std::abs(median / 0.06745f);
	float threshold = standardDeviation * std::sqrt(2.0f * std::log(static_cast<float>(M)));

	MathsUtils::SoftThreshold(array, threshold);
}