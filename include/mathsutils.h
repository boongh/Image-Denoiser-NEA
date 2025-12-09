#pragma once

#include <utility>
#include <vector>
#include <span>

#ifdef DEBUG

#include <iostream>

#endif //DEBUG


namespace MathsUtils
{
    void PosDecompose(
        unsigned int pos,
        unsigned int width,
        unsigned int height,
        int* xstr,
        int* ystr);

    int PosCompose(unsigned int xstr,
        unsigned int ystr,
        unsigned int width);

    // Assumes Y, Cb, and Cr are in their normalized ranges.
    // Uses the inverse of the ITU-R BT.709 standard.
    std::array<float, 3> LRGBtoYCbCr(float R, float G, float B);
    std::array<float, 3> YCbCrtoLRGB(float y, float cb, float cr);

    std::array<float, 3> sRGBtoLRGB(float R, float G, float B);
    std::array<float, 3> LRGBtosRGB(float R, float G, float B);

    float Channel_sRGBtoLRGB(float V);
    float Channel_LRGBtosRGB(float V);

    template <typename T>
    inline T lerp(T a, T b, double t) {
        return static_cast<T>((1 - t) * (double)a + (double)b * t);
    };

    template <typename T>
    int partition(std::span<T> arr, int l, int r) {
        int x = arr[r], i = l;
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
    T QuickSelect(std::span<T> dataSpan, int left, int right, int k) {

        // If k is smaller than the number of elements
        // in the array.
        if (k > 0 && k <= right - left + 1) {

            // Partition the array around the last 
            // element and get the position of the pivot 
            // element in the sorted array.
            int index = MathsUtils::partition(dataSpan, left, right);

            // If position is the same as k.
            if (index - left == k - 1)
                return dataSpan[index];

            // If position is more, recur for the left subarray.
            if (index - left > k - 1)
                return MathsUtils::QuickSelect(dataSpan, left, index - 1, k);

            // Else recur for the right subarray.
            return MathsUtils::QuickSelect(dataSpan, index + 1, right,
                k - index + left - 1);
        }

        // If k is more than the number of elements in the array.
        return dataSpan[0];
    }

    /// <summary>
    /// Soft thresholding operation in place of the input data.
    /// Assuming the T is a signed type and can be negated and compared.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="data"></param>
    /// <param name="value"></param>
    template <typename T>
    void SoftThreshold(std::span<T>& data, T value) {

        #pragma omp parallel for
        for (T& element : data) {

            /*
            Equivalent to
            if (element > value) {
                element -= value;
            }
            else if (element < -value) {
                element += value;
            }
            else {
                element = 0;
			}
            */

            element = (std::abs(element) > value) * (element - std::copysign(1.0f, element) * value);
            
        }
    }

    template<typename T, int size>
    class MathVector {
    private:
    public:
        std::array<T, size> data;

        MathVector() {
            data.fill((T)0);
        }

        template <typename... Args>
        MathVector(Args... args) : data({ static_cast<T>(args)... }) {
            static_assert(sizeof...(args) == size, "Number of arguments must match vector size");
        }

        int GetSize() const {
            return size;
        }

        template<int Bsize>
        MathVector operator+(const MathVector<T, Bsize>& B) const {
            static_assert(size == Bsize, "Size must be equal");
            MathVector result;
            for (int i = 0; i < size; ++i) {
                result.data[i] = this->data[i] + B.data[i];
            }
            return result;
        }

        template<int Bsize>
        MathVector operator-(const MathVector<T, Bsize>& B) const {
            static_assert(size == Bsize, "Size must be equal");
            MathVector result;
            for (int i = 0; i < size; ++i) {
                result.data[i] = this->data[i] - B.data[i];
            }
            return result;
        }

        template<int Bsize>
        MathVector operator*(const T& scalar) const {

            MathVector result;
            for (int i = 0; i < size; ++i) {
                result.data[i] = this->data[i] * scalar;
            }
            return result;
        }

        template<int Bsize>
        MathVector operator/(const T& scalar) const {

            T inverseScalar = 1 / scalar;

            MathVector result;
            for (int i = 0; i < size; ++i) {
                result.data[i] = this->data[i] * inverseScalar;
            }
            return result;
        }

        double Length() const {
            return std::sqrt(LengthSquared());
        }

        double LengthSquared() const {
            double sum = 0.0;
            for (int i = 0; i < size; ++i) {
                sum += static_cast<double>(data[i] * data[i]);
            }
            return sum;
        }
};
}
