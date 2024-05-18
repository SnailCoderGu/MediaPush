#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}
#define WRITE_CAPTURE_AAC

class AacEncoder
{
public:
	AacEncoder();
	~AacEncoder();

	int InitEncode(int sample_rate, int bit_rate, AVSampleFormat sample_fmt,int chanel_layout);
	int Encode(const char* src_buf, int src_len, unsigned char* dst_buf);
	int StopEncode();

	/* check that a given sample format is supported by the encoder */
	static int check_sample_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt)
	{
		const enum AVSampleFormat* p = codec->sample_fmts;

		while (*p != AV_SAMPLE_FMT_NONE) {
			if (*p == sample_fmt)
				return 1;
			p++;
		}
		return 0;
	}

	/* just pick the highest supported samplerate */
	static int check_sample_rate(const AVCodec* codec,int sample_rate)
	{
		const int* p;

		if (!codec->supported_samplerates)
			return 0;

		p = codec->supported_samplerates;
		while (*p) {
			if (*p == sample_rate)
				return 1;
			p++;
		}
		return 0;
	}

	/* select layout with the highest channel count */
	static int select_channel_layout(const AVCodec* codec)
	{
		const uint64_t* p;
		uint64_t best_ch_layout = 0;
		int best_nb_channels = 0;

		if (!codec->channel_layouts)
			return AV_CH_LAYOUT_STEREO;

		p = codec->channel_layouts;
		while (*p) {
			int nb_channels = av_get_channel_layout_nb_channels(*p);

			if (nb_channels > best_nb_channels) {
				best_ch_layout = *p;
				best_nb_channels = nb_channels;
			}
			p++;
		}
		return best_ch_layout;
	}

private:
	AVPacket* pkt = nullptr;
	AVFrame* frame = nullptr;
	AVCodecContext* audioCodecCtx = nullptr;
#ifdef WRITE_CAPTURE_AAC
	FILE* aac_out_file = nullptr;
#endif // WRITE_CAPTURE_YUV
};

