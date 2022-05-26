#include <cmath>

#include "spy/common/am/icons/resample.hpp"
#include "spy/common/am/thirdparty/resample/resampler.h"
#include "thirdparty/image/png/lodepng.h"

#include "bsw/assert.hpp"

namespace
{
	struct gamma_table_s
	{
		enum
		{
			linear_to_srgb_table_size = 4096
		};

		gamma_table_s(float source_gamma);

		float srgb_to_linear[256];

		unsigned char linear_to_srgb[linear_to_srgb_table_size];
	};

	gamma_table_s::gamma_table_s(float source_gamma)
	{
		for (int i = 0; i < 256; ++i)
		{
			srgb_to_linear[i] = (float)pow(i * 1.0f / 255.0f, source_gamma);
		}

		const float inv_linear_to_srgb_table_size = 1.0f / linear_to_srgb_table_size;
		const float inv_source_gamma = 1.0f / source_gamma;

		for (int i = 0; i < linear_to_srgb_table_size; ++i)
		{
			int k = (int)(255.0f * std::pow(i * inv_linear_to_srgb_table_size, inv_source_gamma) + .5f);
			if (k < 0) k = 0; else if (k > 255) k = 255;
			linear_to_srgb[i] = (unsigned char)k;
		}
	}

	class auto_resampler
	{
	public:
		auto_resampler(Resampler* objects[], int num)
			: obj(objects),
			n(num)
		{}

		~auto_resampler()
		{
			for (int i = 0; i < n; i++)
			{
				if (obj[i])
				{
					delete obj[i];
				}
			}
		}
	private:
		auto_resampler() = delete;
		auto_resampler(const auto_resampler&) = delete;
		auto_resampler& operator = (const auto_resampler&) = delete;

		Resampler** obj;
		int n;
	};

} // anonymous ns

namespace spy
{
	namespace am
	{
		bool reduce_png(std::vector <unsigned char>& input, unsigned max_size)
		{
			const float source_gamma = 1.75f;

			// Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
			const float filter_scale = 1.0f;//.75f;

			static const gamma_table_s gamma(source_gamma);

			std::vector <unsigned char> image;
			unsigned src_width;
			unsigned src_height;
			const unsigned rc = lodepng::decode(image, src_width, src_height, input, LCT_RGBA, 8);

			if (src_width < max_size && src_height < max_size)
			{
				return true;
			}

			unsigned dst_width = max_size;
			unsigned dst_height = max_size;

			if (rc)
			{
				return false;
			}

			unsigned char* pSrc_image = image.data();

			const int n = 4; // channels
			Resampler* resamplers[n] = { 0 };
			std::vector<float> samples[n];
			auto_resampler guard(resamplers, n);
			const char* pFilter = "blackman";//RESAMPLER_DEFAULT_FILTER;
											 // Now create a Resampler instance for each component to process. The first instance will create new contributor tables, which are shared by the resamplers 
											 // used for the other components (a memory and slight cache efficiency optimization).
			resamplers[0] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, NULL, NULL, filter_scale, filter_scale);
			samples[0].resize(src_width);
			for (int i = 1; i < n; i++)
			{
				resamplers[i] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(), resamplers[0]->get_clist_y(), filter_scale, filter_scale);
				samples[i].resize(src_width);
			}


			std::vector<unsigned char> dst_image(dst_width * n * dst_height);

			const int src_pitch = src_width * n;
			const int dst_pitch = dst_width * n;
			unsigned int dst_y = 0;

			for (unsigned int src_y = 0; src_y < src_height; src_y++)
			{
				const unsigned char* pSrc = &pSrc_image[src_y * src_pitch];

				for (unsigned int x = 0; x < src_width; x++)
				{
					for (int c = 0; c < n; c++)
					{
						if ((c == 3) || ((n == 2) && (c == 1)))
							samples[c][x] = *pSrc++ * (1.0f / 255.0f);
						else
							samples[c][x] = gamma.srgb_to_linear[*pSrc++];
					}
				}

				for (int c = 0; c < n; c++)
				{
					if (!resamplers[c]->put_line(&samples[c][0]))
					{
						return false;
					}
				}

				for (; ; )
				{
					int comp_index;
					for (comp_index = 0; comp_index < n; comp_index++)
					{
						const float* pOutput_samples = resamplers[comp_index]->get_line();
						if (!pOutput_samples)
							break;

						const bool alpha_channel = (comp_index == 3) || ((n == 2) && (comp_index == 1));
						BSW_ASSERT(dst_y < dst_height);
						unsigned char* pDst = &dst_image[dst_y * dst_pitch + comp_index];

						for (unsigned int x = 0; x < dst_width; x++)
						{
							if (alpha_channel)
							{
								int c = (int)(255.0f * pOutput_samples[x] + .5f);
								if (c < 0) c = 0; else if (c > 255) c = 255;
								*pDst = (unsigned char)c;
							}
							else
							{
								int j = (int)(gamma.linear_to_srgb_table_size * pOutput_samples[x] + .5f);
								if (j < 0) j = 0; else if (j >= gamma.linear_to_srgb_table_size) j = gamma.linear_to_srgb_table_size - 1;
								*pDst = gamma.linear_to_srgb[j];
							}

							pDst += n;
						}
					}
					if (comp_index < n)
						break;

					dst_y++;
				}
			}
			input.resize(0);

			lodepng::encode(input, dst_image, dst_width, dst_height, LCT_RGBA, 8);

			return true;
		}
	} // ns am
} // ns spy
