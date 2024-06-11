#include "RtmpPush.h"
#include <QDebug>

void RtmpPush::OpenFormat(std::string url)
{

	url_ = url;

	av_log_set_level(AV_LOG_VERBOSE);
	av_register_all();
	avformat_network_init();

	avformat_alloc_output_context2(&pFormatCtx, NULL, "flv", url.c_str());
	if (!pFormatCtx)
	{
		qDebug() << "avformat alloc failed";
		exit(1);
	}

	pFormatCtx->duration = 0;

}

void RtmpPush::InitVideoCodePar(AVMediaType codec_type, AVCodecID code_id, int width, int height, int fps, int format,
	const uint8_t* extradata,
	int extextradata_size)
{
	video_stream = avformat_new_stream(pFormatCtx, NULL);
	if (!video_stream) {
		qDebug() << "Could not create video stream";
		exit(1);
	}
	video_stream->codecpar->codec_type = codec_type;
	video_stream->codecpar->codec_id = code_id;
	video_stream->codecpar->width = width;  // 设置宽度
	video_stream->codecpar->height = height;  // 设置高度
	video_stream->codecpar->format = format;
	video_stream->codecpar->bit_rate = 2000000;
	i_fps = fps;
	if (extextradata_size > 0) {
		// Copy extradata (SPS and PPS) from the codec context to the stream
		video_stream->codecpar->extradata = (uint8_t*)av_mallocz(extextradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(video_stream->codecpar->extradata, extradata, extextradata_size);
		video_stream->codecpar->extradata_size = extextradata_size;
	}

}

void RtmpPush::InitAudioCodecPar(AVMediaType codec_type, AVCodecID code_id, int sample_rate, int channels, int format, 
	const uint8_t* extradata,
	int extextradata_size)
{

	// Create audio stream
	audio_stream = avformat_new_stream(pFormatCtx, NULL);
	if (!audio_stream) {
		fprintf(stderr, "Could not create audio stream\n");
		exit(1);
	}
	audio_stream->codecpar->codec_type = codec_type;
	audio_stream->codecpar->codec_id = code_id;
	audio_stream->codecpar->sample_rate = sample_rate;  // 设置采样率
	audio_stream->codecpar->channels = channels;         // 设置声道数
	audio_stream->codecpar->channel_layout = av_get_default_channel_layout(2);
	audio_stream->codecpar->format = format;
	audio_stream->codecpar->profile = FF_PROFILE_AAC_HE;

	audio_stream->time_base = { 1, sample_rate };

	i_sample_rate = sample_rate;

	// Copy extradata (SPS and PPS) from the codec context to the stream
	if (extextradata_size > 0) {
		audio_stream->codecpar->extradata = (uint8_t*)av_mallocz(extextradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(audio_stream->codecpar->extradata, extradata, extextradata_size);
		audio_stream->codecpar->extradata_size = extextradata_size;
	}
}

void RtmpPush::WriteHeader()
{

	if (avio_open(&pFormatCtx->pb, url_.c_str(), AVIO_FLAG_WRITE) < 0) {
		qDebug() << "Could not open output URL " << url_.c_str();
		exit(1);
	}

	//AVOutputFormat* av_out_fomat = pFormatCtx->oformat;

	av_dump_format(pFormatCtx, 0, url_.c_str(), 1);

	// 写入头文件
	if (avformat_write_header(pFormatCtx, NULL) < 0) {
		qDebug() << "Error occurred when opening output URL";
		exit(1);
	}

	auto now  = std::chrono::system_clock::now();
	start_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void RtmpPush::PushPacket(MediaType type,uint8_t* data, int size) {

	
	std::lock_guard<std::mutex> lock(ffmpeg_mutex);

	bool is_video = (type == MediaType::VIDEO);
	
	AVStream* stream = is_video ? video_stream : audio_stream;

	AVPacket packet;
	av_init_packet(&packet);
	AVRational pkt_rat;

	//if (is_video) {
	//	packet.pts = packet.dts = video_pts;
	//	video_pts += 1;
	//	pkt_rat.num = 1;
	//	pkt_rat.den = i_fps;
	//}
	//else {
	//	packet.pts = packet.dts = audio_pts;
	//	audio_pts += 1024; // For AAC, 1024 samples per frame
	//	pkt_rat.num = 1;
	//	pkt_rat.den = i_sample_rate;
	//}

	
	auto now = std::chrono::system_clock::now();
	auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	packet.dts = packet.pts = (double)(now_time - start_time) / (av_q2d(is_video ? video_stream->time_base : audio_stream->time_base) * 1000);

	//packet.pts = av_rescale_q(packet.pts, pkt_rat, is_video ? video_stream->time_base : audio_stream->time_base);
	//packet.dts = av_rescale_q(packet.dts, pkt_rat, is_video ? video_stream->time_base : audio_stream->time_base);
	//packet.duration = av_rescale_q(1, pkt_rat, is_video ? video_stream->time_base : audio_stream->time_base);

	packet.data = data;
	packet.size = size;
	packet.stream_index = stream->index;

	if (av_interleaved_write_frame(pFormatCtx, &packet) < 0) {
		qDebug() << "Error muxing" << (is_video ?" video":"audio ") << "packet\n";
	}
	
}

void RtmpPush::Close()
{
	av_write_trailer(pFormatCtx);

	// Free the streams
	if (video_stream) {
		avcodec_parameters_free(&video_stream->codecpar);
		video_stream = nullptr;
	}

	if (audio_stream) {
		avcodec_parameters_free(&audio_stream->codecpar);
		audio_stream = nullptr;
	}

	// Close the input file and free the format context
	if (pFormatCtx && !(pFormatCtx->oformat->flags & AVFMT_NOFILE)) {
		avio_closep(&pFormatCtx->pb);
	}


	avio_closep(&pFormatCtx->pb);
	
	if (pFormatCtx) {
		avformat_free_context(pFormatCtx);
		pFormatCtx = nullptr;
	}
}
