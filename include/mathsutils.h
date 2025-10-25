#pragma once

#include <array>
#include <cmath>

inline void PosDecompose(
    unsigned int pos,
    unsigned int width,
    unsigned int height,
    int* xstr,
    int* ystr);

inline int PosCompose(unsigned int xstr,
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
