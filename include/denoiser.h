#pragma once

#include <span>
#include <algorithm>
#include <FileFormats.h>
#include <stdexcept>
#include <cmath>
#include <print>
#include <Eigen/Dense>


class Denoiser {
public:
	/// <summary>
	/// Mean Linear filtering (LF) denoising algorithm.
	/// </summary>
	/// <param name="src"></param>
	/// <param name="dst"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <param name="strn"></param>
	static std::vector<PixelRGBA> SmoothLF(std::span<const PixelRGBA> src,
		unsigned int width, unsigned int height, 
		int halfWidth, int halfHeight, 
		double strn);

	static std::vector<PixelRGBA> BilateralFilter(std::span<const PixelRGBA> src,
		unsigned int width, unsigned int height,
		int halfWidth, int halfHeight, double strnSpatial, double strnIntensity);

    static std::vector<PixelRGBA> FastBilateralFilterApproximation(std::span<const PixelRGBA> src,
        unsigned int width, unsigned int height,
        int halfWidth, int halfHeight, double strnSpatial, double strnIntensity, double threashold);

    static std::vector<PixelRGBA> FastBFApprox2(std::span<const PixelRGBA> src,
        unsigned int width, unsigned int height,
        int halfWidth, int halfHeight, double strnSpatial, double strnIntensity);

    static void GaussianBlur(std::span<const PixelRGBA> src,
        std::span<PixelRGBA>& dst,
        unsigned int width, unsigned int height,
		int halfWidth, int halfHeight, double strn);

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
            for(int i = 0; i < size; ++i) {
                sum += static_cast<double>(data[i] * data[i]);
			}
            return sum;
        }
    };


	class Bilateral {
	public:
		static inline double RangeAttenuation(double distance, double strn);
		static inline double IntensityAttenuation(double distance, double strn);

        template <typename T, int size>
        static inline double VectorAttenuation(
            const MathVector<T, size>& v1,
            const MathVector<T, size>& v2,
            double inverseSD) {
			MathVector<T, size> diff = v1 - v2;
            double distanceSquared = diff.LengthSquared();
            return std::exp(-distanceSquared / (2 * inverseSD * inverseSD));
        }
	};

	static void PosDecompose(
		unsigned int pos,
		unsigned int width,
		unsigned int height,
		int* xstr,
		int* ystr);

	static inline int PosCompose(unsigned int xstr,
		unsigned int ystr,
		unsigned int width);

	template <typename T>
	static inline T lerp(T a, T b, double t) {
		return static_cast<T>((1 - t) * (double)a + (double)b * t);
	};
};