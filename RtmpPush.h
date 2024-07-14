#pragma once
#include <string>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavutil/mem.h>  // 包含 av_mallocz 的头文件
}

class RtmpPush
{
public:
	enum MediaType
	{
		VIDEO = 0,
		AUDIO = 1
	};
public:
	void OpenFormat(std::string url);
	void InitVideoCodePar(AVMediaType codec_type, AVCodecID code_id, int width, int height, int fps, int format, const uint8_t* extradata,
		int extextradata_size);
	void InitAudioCodecPar(AVMediaType codec_type, AVCodecID code_id, int sample_rate, int channels, int format, const uint8_t* extradata, 
		int extextradata_size);
	void WriteHeader();
	void PushPacket(MediaType type, uint8_t* data,int size);

	void Close();

	//static void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt, const char* tag)
	//{
	//	AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

	//	printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
	//		tag,
	//		av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
	//		av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
	//		av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
	//		pkt->stream_index);
	//}

private:
	AVFormatContext* pFormatCtx = nullptr;
	AVStream* audio_stream;
	AVStream* video_stream;

	std::mutex ffmpeg_mutex;

	uint64_t start_time;

	std::string url_;

	int i_fps;
	int i_sample_rate;

	int64_t video_pts;
	int64_t audio_pts;


	//AVBSFContext* bsf_ctx = nullptr;
};

