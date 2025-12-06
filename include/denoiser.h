#pragma once

#include <span>
#include <algorithm>
#include <FileFormats.h>
#include <stdexcept>
#include <cmath>
#include <print>
#include <Eigen/Dense>
#include "mathsutils.h"

namespace Denoiser {

    using namespace MathsUtils;

    template<typename T>
    struct Thresholder {
        virtual void SoftThreshold(std::span<T> array) {
        };
        virtual void HardThreshold(std::span<T> array) {
        };
    };

    struct VisuShrink : public Thresholder<float> {
        void SoftThreshold(std::span<float> array) override;
    };


	/// <summary>
	/// Mean Linear filtering (LF) denoising algorithm.
	/// </summary>
	/// <param name="src"></param>
	/// <param name="dst"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <param name="strn"></param>
	int SmoothLF(std::span<PixelRGBA> src,
		unsigned int width, unsigned int height, 
		int halfWidth, int halfHeight, 
		double strn);

	int BilateralFilter(std::span<const PixelRGBA> src,
		unsigned int width, unsigned int height,
		int halfWidth, int halfHeight, double strnSpatial, double strnIntensity);

    std::vector<PixelRGBA> FastBilateralFilterApproximation(std::span<const PixelRGBA> src,
        unsigned int width, unsigned int height,
        int halfWidth, int halfHeight, double strnSpatial, double strnIntensity, double threashold);

    std::vector<PixelRGBA> FastBFApprox2(std::span<const PixelRGBA> src,
        unsigned int width, unsigned int height,
        int halfWidth, int halfHeight, double strnSpatial, double strnIntensity);

    void GaussianBlur(std::span<const PixelRGBA> src,
        std::span<PixelRGBA>& dst,
        unsigned int width, unsigned int height,
		int halfWidth, int halfHeight, double strn);

    int DWT(std::span<PixelRGBA> src,
        unsigned int width, unsigned int height,
        int decimationLevel);


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
            std::array<const double, size> dec_lo;
            std::array<const double, size> dec_hi;
            std::array<const double, size> rec_lo;
            std::array<const double, size> rec_hi;

            
            Wavelet(
                const std::array<const double, size>& dlo,
                const std::array<const double, size>& dhi,
                const std::array<const double, size>& rlo,
                const std::array<const double, size>& rhi)
                : dec_lo(dlo), dec_hi(dhi), rec_lo(rlo), rec_hi(rhi) {
            }
        };


#pragma region wavelets values

        inline static const Wavelet<4> sym2 = Wavelet<4>(
            std::array<const double, 4>{-0.12940952255092, 0.22414386804186, 0.83651630373747, 0.48296291314469},
            std::array<const double, 4>{-0.48296291314469, 0.83651630373747, -0.22414386804186, -0.12940952255092},
            std::array<const double, 4>{0.48296291314469, 0.83651630373747, 0.22414386804186, -0.12940952255092},
            std::array<const double, 4>{-0.12940952255092, -0.22414386804186, 0.83651630373747, -0.48296291314469}
        );

        inline static const Wavelet<6> sym3 = Wavelet<6>(
            std::array<const double, 6>{0.03522629188210, -0.08544127388224, -0.13501102001039, 0.45987750211933, 0.80689150931334, 0.33267055295096},
            std::array<const double, 6>{-0.33267055295096, 0.80689150931334, -0.45987750211933, -0.13501102001039, 0.08544127388224, 0.03522629188210},
            std::array<const double, 6>{0.33267055295096, 0.80689150931334, 0.45987750211933, -0.13501102001039, -0.08544127388224, 0.03522629188210},
            std::array<const double, 6>{0.03522629188210, 0.08544127388224, -0.13501102001039, -0.45987750211933, 0.80689150931334, -0.33267055295096}
        );

        inline static const Wavelet<8> sym4 = Wavelet<8>(
            std::array<const double, 8>{-0.07576571478927, -0.02963552764600, 0.49761866763202, 0.80373875180592, 0.29785779560528, -0.09921954357685, -0.01260396726204, 0.03222310060404},
            std::array<const double, 8>{-0.03222310060404, -0.01260396726204, 0.09921954357685, 0.29785779560528, -0.80373875180592, 0.49761866763202, 0.02963552764600, -0.07576571478927},
            std::array<const double, 8>{0.03222310060404, -0.01260396726204, -0.09921954357685, 0.29785779560528, 0.80373875180592, 0.49761866763202, -0.02963552764600, -0.07576571478927},
            std::array<const double, 8>{-0.07576571478927, 0.02963552764600, 0.49761866763202, -0.80373875180592, 0.29785779560528, 0.09921954357685, -0.01260396726204, -0.03222310060404}
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

            int ExpandTree(int decimationLevel);
            int CollapseTree(DecNode::ReconMode mode = DecNode::ReconMode::Full);

            void Thresholding(Thresholder<float>& thresholder);

            RGBAImageI GetImageRGB(float Y = 1.0, float Cb = 1.0, float Cr = 1.0, float r = 1.0, float g = 1.0, float b = 1.0);
			RGBAImageI GetImageGray(float Y = 1.0);
            std::vector<std::array<float, 3>> GetImageYCbCr();


            std::vector<std::array<float, 3>> rootImage; // Root image in YCrCb
            std::vector<int> rootAlpha; // Root Alpha since YCrCb doesn't support alpha

            std::shared_ptr<DecNode> rootNode;
        };
    };
};