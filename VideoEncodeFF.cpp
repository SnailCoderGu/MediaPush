#include "VideoEncodeFF.h"
#include <QDebug>
#include <stdint.h>
//#include "x264.h"


int VideoEncodeFF::InitEncode(int width, int height, int fps, int bitrate, const char* profile)
{
	if (width <= 0 || height <= 0)
		return -1;

	int ret = 0;
	av_register_all();

	AVCodec* codec = NULL;
	while ((codec = av_codec_next(codec))) {
		if (codec->encode2 && codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
			qDebug() << "Codec Name :" << codec->name;
			qDebug() << "Type: " << av_get_media_type_string(codec->type);
			qDebug() << "Description: " << (codec->long_name ? codec->long_name : codec->name);
			qDebug() << "---";
		}
	}

	//codec = avcodec_find_encoder_by_name("libx264");
	codec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_H264);


	if (codec == NULL) {
		//qDebug() << "cannot find video codec id: " << codec->id;
		return -1;
	}

	qDebug() << "codec name: " << codec->name;
	qDebug() << "codec long name: " << codec->long_name;

	videoCodecCtx = avcodec_alloc_context3(codec);
	if (videoCodecCtx == nullptr)
	{
		qDebug() << "alloc context3 afild: " << codec->long_name;
		return -1;
	}

	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	// 设置编码参数
	videoCodecCtx->bit_rate = bitrate;
	videoCodecCtx->width = width;
	videoCodecCtx->height = height;
	videoCodecCtx->time_base = { 1,fps };
	videoCodecCtx->framerate = { fps,1 };
	videoCodecCtx->gop_size = 10;
	videoCodecCtx->max_b_frames = 0;
	videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;


	std::string strProfile = profile;
	if (strProfile == "main")
	{
		videoCodecCtx->profile = FF_PROFILE_H264_MAIN;
	}
	else if (strProfile == "base")
	{
		videoCodecCtx->profile = FF_PROFILE_H264_BASELINE;
	}
	else
	{
		videoCodecCtx->profile = FF_PROFILE_H264_HIGH;
	}

	ret = av_opt_set(videoCodecCtx->priv_data, "preset", "ultrafast", 0);
	if (ret != 0)
	{
		qDebug() << "set opt preset error";
	}
	ret = av_opt_set(videoCodecCtx->priv_data,"tune","zerolatency", 0);
	if (ret != 0)
	{
		qDebug() << "set opt tune error";
	}

	//x264_param_t* x264_param = (x264_param_t*)videoCodecCtx->priv_data;
	//x264_param_default_preset(x264_param, "ultrafast", "zerolatency");
	//x264_param_apply_profile(x264_param, "high");

	// 打开编码器
	ret = avcodec_open2(videoCodecCtx, codec, NULL);
	if (ret < 0) {
		qDebug() << "avcodec_open2 filed ";
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = videoCodecCtx->pix_fmt;
	frame->width = videoCodecCtx->width;
	frame->height = videoCodecCtx->height;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

#ifdef WRITE_CAPTURE_264
	if (!h264_out_file) {
		h264_out_file = fopen("ouput.264", "wb");
		if (h264_out_file == nullptr)
		{
			qDebug() << "Open 264 file failed";
		}
	}
#endif // WRITE_CAPTURE_264

    return 0;
}

unsigned int VideoEncodeFF::Encode(unsigned char* src_buf, unsigned char* dst_buf)
{
	int ylen = frame->linesize[0] * videoCodecCtx->coded_height;
	int ulen = frame->linesize[1] * videoCodecCtx->coded_height /2 ;
	int vlen = frame->linesize[2] * videoCodecCtx->coded_height / 2;
	memcpy(frame->data[0], src_buf, ylen);
	memcpy(frame->data[1], src_buf + ylen, ulen);
	memcpy(frame->data[2], src_buf + ylen+ulen, vlen);

	frame->pts = pts;
	pts++;

	int ret = avcodec_send_frame(videoCodecCtx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(videoCodecCtx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return 1;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}

#ifdef WRITE_CAPTURE_264
		fwrite(pkt->data, 1, pkt->size, h264_out_file);
#endif // WRITE_CAPTURE_264
		if (dst_buf != nullptr)
		{
			memcpy(pkt->data, dst_buf, pkt->size);
		}
		av_packet_unref(pkt);
	}

    return 0;
}

int VideoEncodeFF::StopEncode()
{
	avcodec_free_context(&videoCodecCtx);
	av_frame_free(&frame);
	av_packet_free(&pkt);

#ifdef WRITE_CAPTURE_264
	if (h264_out_file) {
		fclose(h264_out_file);
		h264_out_file = nullptr;
	}
#endif

    return 0;
}
