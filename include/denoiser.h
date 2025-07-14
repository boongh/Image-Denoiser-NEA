#pragma once

#include <span>
#include <algorithm>
#include <FileFormats.h>
#include <stdexcept>
#include <cmath>
#include <print>

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

	/// <summary>
	template<typename T>
    class MathVector5 {
    private:
        int m_size = 5;
    public:
        // Members can be of different types
        T m0;
        T m1;
        T m2;
        T m3;
        T m4;

        MathVector5()
            : m0(0.0), m1(0), m2(0.0f), m3(0), m4(0L) {}

        MathVector5(T v0, T v1, T v2, T v3, T v4)
            : m0(v0), m1(v1), m2(v2), m3(v3), m4(v4) {}

        int GetSize() const {
            return m_size;
        }

        MathVector5 operator+(const MathVector5& B) const {
            return MathVector5(
                m0 + B.m0,
                m1 + B.m1,
                m2 + B.m2,
                m3 + B.m3,
                m4 + B.m4
            );
        }

        MathVector5 operator-(const MathVector5& B) const {
            return MathVector5(
                m0 - B.m0,
                m1 - B.m1,
                m2 - B.m2,
                m3 - B.m3,
                m4 - B.m4
            );
        }

        MathVector5 operator*(const MathVector5& B) const {
            return MathVector5(
                m0 * B.m0,
                m1 * B.m1,
                m2 * B.m2,
                m3 * B.m3,
                m4 * B.m4
            );
        }

        MathVector5 operator/(const MathVector5& B) const {
            if (B.m0 == 0.0 || B.m1 == 0 || B.m2 == 0.0f || B.m3 == 0 || B.m4 == 0L) {
                throw std::runtime_error("Division by zero in vector division");
            }
            return MathVector5(
                m0 / B.m0,
                m1 / B.m1,
                m2 / B.m2,
                m3 / B.m3,
                m4 / B.m4
            );
        }

        double Length() const {
            return std::sqrt(
                m0 * m0 +
                static_cast<double>(m1 * m1) +
                static_cast<double>(m2 * m2) +
                static_cast<double>(m3 * m3) +
                static_cast<double>(m4 * m4)
            );
        }

        double LengthSquared() const {
            return (
                m0 * m0 +
                static_cast<double>(m1 * m1) +
                static_cast<double>(m2 * m2) +
                static_cast<double>(m3 * m3) +
                static_cast<double>(m4 * m4)
            );
        }
    };
//
//	/// Small vector class, may change for Eigen instead
//	/// </summary>
//	/// <typeparam name="type"></typeparam>
//	/// <typeparam name="size"></typeparam>
//	template <typename T, int size>
//	class MathVector {
//	private:
//		int m_size = size;
//	public:
//
//		T m_data[size];
//
//		template<typename... Args>
//		MathVector(Args... args) : m_data{ args... } {
//			static_assert(sizeof...(args) == size, "Number of arguments must match vector size");
//		};
//
//		MathVector() {
//			for (int i = 0; i < size; ++i) {
//				m_data[i] = T();
//			}
//		}
//
//		int GetSize() {
//			return m_size;
//		}
//
//		template<int otherSize>
//		MathVector<T, size> operator + (const MathVector<T, otherSize>& B) const {
//			static_assert(size == otherSize, "Vector sizes must match");
//			MathVector result;
//			for(int i = 0; i < size; ++i) {
//				result.m_data[i] = m_data[i] + B.m_data[i];
//			}
//			return result;
//		}
//
//		template<int otherSize>
//		MathVector<T, size> operator - (const MathVector<T, otherSize>& B) const {
//			static_assert(size == otherSize, "Vector sizes must match");
//			MathVector result;
//			for(int i = 0; i < size; ++i) {
//				result.m_data[i] = m_data[i] - B.m_data[i];
//			}
//			return result;
//		}
//
//		template<int otherSize>
//		MathVector<T, size> operator * (const MathVector<T, otherSize>& B) const {
//			static_assert(size == otherSize, "Vector sizes must match");
//			MathVector result;
//			for(int i = 0; i < size; ++i) {
//				result.m_data[i] = m_data[i] * B.m_data[i];
//			}
//			return result;
//		}
//
//		template<int otherSize>
//		MathVector<T, size> operator / (const MathVector<T, otherSize>& B) const {
//			static_assert(size == otherSize, "Vector sizes must match");
//			MathVector result;
//			for(int i = 0; i < size; ++i) {
//				if (B.m_data[i] == 0) {
//					throw std::runtime_error("Division by zero in vector division");
//				}
//				result.m_data[i] = m_data[i] / B.m_data[i];
//			}
//			return result;
//		}
//
//		//MathVector<T, size> operator = (const MathVector<T, size>&& B) {
//		//	MathVector<T, size> result;
//		//	std::move(&B, &B + sizeof(B), &result);
//		//	return result;
//		//}
//		//
//		//MathVector(const MathVector<T, size>&& B) {
//		//	MathVector<T, size> result;
//		//	std::move(&B, &B + sizeof(B), &result);
//		//	return result;
//		//}
//		//
//		//MathVector(const MathVector<T, size>& B) {
//		//	MathVector<T, size> result;
//		//	std::copy(&B, &B + sizeof(B), &result);
//		//	return result;
//		//}
//
//		double Length() const {
//			double sum = 0.0;
//			for (int i = 0; i < size; ++i) {
//				sum += m_data[i] * m_data[i];
//			}
//			return std::sqrt(sum);
//		}
//
//		double LengthSquared() const {
//			double sum = 0.0;
//			for (int i = 0; i < size; ++i) {
//				sum += m_data[i] * m_data[i];
//			}
//			return sum;
//		}
//	};

	class Bilateral {
	public:
		static inline double RangeAttenuation(double distance, double strn);
		static inline double IntensityAttenuation(double distance, double strn);

		template <typename T>
		static inline double VectorAttenuation(MathVector5<T> v1, MathVector5<T> v2, double strn) {
			double distanceSquared = (v1 - v2).LengthSquared();
			return std::exp(-distanceSquared/ (2 * strn * strn));
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
		return static_cast<T>((1 - t) * a + b * t);
	};
};