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

    static void DWT(std::span<const PixelRGBA> src,
        std::span<PixelRGBA>& dst,
        unsigned int width, unsigned int height);

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

    struct DWT {
    
        template <int size>
        class Wavelet {
        public:
            std::array<double, size> dec_lo;
            std::array<double, size> dec_hi;
            std::array<double, size> rec_lo;
            std::array<double, size> rec_hi;

            
            Wavelet(
                const std::array<double, size>& dlo,
                const std::array<double, size>& dhi,
                const std::array<double, size>& rlo,
                const std::array<double, size>& rhi)
                : dec_lo(dlo), dec_hi(dhi), rec_lo(rlo), rec_hi(rhi) {
            }
        };


#pragma region wavelets values

        inline static const Wavelet<4> sym2 = Wavelet<4>(
            std::array<double, 4>{-0.12940952255092, 0.22414386804186, 0.83651630373747, 0.48296291314469},
            std::array<double, 4>{-0.48296291314469, 0.83651630373747, -0.22414386804186, -0.12940952255092},
            std::array<double, 4>{0.48296291314469, 0.83651630373747, 0.22414386804186, -0.12940952255092},
            std::array<double, 4>{-0.12940952255092, -0.22414386804186, 0.83651630373747, -0.48296291314469}
        );

        inline static const Wavelet<6> sym3 = Wavelet<6>(
            std::array<double, 6>{0.03522629188210, -0.08544127388224, -0.13501102001039, 0.45987750211933, 0.80689150931334, 0.33267055295096},
            std::array<double, 6>{-0.33267055295096, 0.80689150931334, -0.45987750211933, -0.13501102001039, 0.08544127388224, 0.03522629188210},
            std::array<double, 6>{0.33267055295096, 0.80689150931334, 0.45987750211933, -0.13501102001039, -0.08544127388224, 0.03522629188210},
            std::array<double, 6>{0.03522629188210, 0.08544127388224, -0.13501102001039, -0.45987750211933, 0.80689150931334, -0.33267055295096}
        );

        inline static const Wavelet<8> sym4 = Wavelet<8>(
            std::array<double, 8>{-0.07576571478927, -0.02963552764600, 0.49761866763202, 0.80373875180592, 0.29785779560528, -0.09921954357685, -0.01260396726204, 0.03222310060404},
            std::array<double, 8>{-0.03222310060404, -0.01260396726204, 0.09921954357685, 0.29785779560528, -0.80373875180592, 0.49761866763202, 0.02963552764600, -0.07576571478927},
            std::array<double, 8>{0.03222310060404, -0.01260396726204, -0.09921954357685, 0.29785779560528, 0.80373875180592, 0.49761866763202, -0.02963552764600, -0.07576571478927},
            std::array<double, 8>{-0.07576571478927, 0.02963552764600, 0.49761866763202, -0.80373875180592, 0.29785779560528, 0.09921954357685, -0.01260396726204, -0.03222310060404}
        );



#pragma endregion


        struct DecNode {

            DecNode(unsigned int width, unsigned int height, long layer);

            std::vector<float> brightnessData; //nodes only deals with one channel at a time.
            std::shared_ptr<DecNode> low;
            std::shared_ptr<DecNode> high;

            long layer;

            unsigned int width;
            unsigned int height;


            enum ReconMode
            {
                Low = 1,
                High = 2,
                Full = 3,
            };

            /// <summary>
            /// Decompose the current node into childnodes
            /// </summary>
            /// <param name="direction">0 for horizontal, 1 for vertical</param>
            /// <returns>
            ///     0 successful
            ///     1 invalid size
            /// </returns>
            int DecomposeNode(int wavelet);


            /// <summary>
            /// Recompose the current node into childnodes
            /// </summary>
            /// <param name="direction">0 for horizontal, 1 for vertical</param>
            /// <returns>
            ///     0 successful
            ///     1 failed
            /// </returns>
            int RecomposeNode(ReconMode mode = ReconMode::Full);
        };

        struct DecTree {
            bool expanded;

            DecTree();

            DecTree(std::span<const PixelRGBA> src, unsigned int width, unsigned int height);

            int ExpandTree();
            int CollapseTree(DecNode::ReconMode mode = DecNode::ReconMode::Full);
            RGBAImageI GetImageRGB(float Y = 1.0, float Cb = 1.0, float Cr = 1.0, float r = 1.0, float g = 1.0, float b = 1.0);
			RGBAImageI GetImageGray(float Y = 1.0);
            std::vector<std::array<float, 3>> GetImageYCbCr();


            std::vector<std::array<float, 3>> rootImage; // Root image in YCrCb
            std::vector<int> rootAlpha; // Root Alpha since YCrCb doesn't support alpha

            std::shared_ptr<DecNode> rootNode;
        };
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

    // Assumes Y, Cb, and Cr are in their normalized ranges.
    // Uses the inverse of the ITU-R BT.709 standard.
    static std::array<float, 3> LRGBtoYCbCr(float R, float G, float B);
    static std::array<float, 3> YCbCrtoLRGB(float y, float cb, float cr);

    static std::array<float, 3> sRGBtoLRGB(float R, float G, float B);
    static std::array<float, 3> LRGBtosRGB(float R, float G, float B);

    static float Channel_sRGBtoLRGB(float V);
    static float Channel_LRGBtosRGB(float V);

	template <typename T>
	static inline T lerp(T a, T b, double t) {
		return static_cast<T>((1 - t) * (double)a + (double)b * t);
	};
};