#include "AacEncoder.h"
#include <QDebug>

AacEncoder::AacEncoder()
{
}

AacEncoder::~AacEncoder()
{
}

int AacEncoder::InitEncode(int sample_rate, int bit_rate, AVSampleFormat sample_fmt, int chanel_layout)
{
	//avcodec_register_all();
	av_register_all();

	const AVCodec* codec = nullptr;
	while ((codec = av_codec_next(codec))) {
		if (codec->encode2 && codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
			qDebug() << "Codec Name :" << codec->name;
			qDebug() << "Type: " << av_get_media_type_string(codec->type);
			qDebug() << "Description: " << (codec->long_name ? codec->long_name : codec->name);
			qDebug() << "---";
		}
	}

	
	/* find the MP2 encoder */
	codec = avcodec_find_encoder_by_name("libfdk_aac");
	//codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	qDebug() << "codec name: " << codec->name;
	qDebug() << "codec long name: " << codec->long_name;



	const enum AVSampleFormat* p = codec->sample_fmts;
	while (*p != AV_SAMPLE_FMT_NONE) {
		qDebug() << "supoort codec fmt : " << av_get_sample_fmt_name(*p);
		p++;
	}
	

	audioCodecCtx = avcodec_alloc_context3(codec);
	if (!audioCodecCtx) {
		fprintf(stderr, "Could not allocate audio codec context\n");
		exit(1);
	}


	//打印看到只支持 AV_SAMPLE_FMT_S16，所以这里写死
	


	audioCodecCtx->sample_rate = sample_rate;
	audioCodecCtx->channel_layout = chanel_layout;
	audioCodecCtx->channels = av_get_channel_layout_nb_channels(audioCodecCtx->channel_layout);
	audioCodecCtx->sample_fmt = sample_fmt;
	audioCodecCtx->bit_rate = bit_rate;

	//检查是否支持fmt
	if (!check_sample_fmt(codec, audioCodecCtx->sample_fmt)) {
		//fprintf(stderr, "Encoder does not support sample format %s",av_get_sample_fmt_name(audioCodecCtx->sample_fmt));
		qDebug() << "Encoder does not support sample format " << av_get_sample_fmt_name(audioCodecCtx->sample_fmt);
		exit(1);
	}

	if (!check_sample_rate(codec, audioCodecCtx->sample_rate)) {
		//fprintf(stderr, "Encoder does not support sample format %s",av_get_sample_fmt_name(audioCodecCtx->sample_fmt));
		qDebug() << "Encoder does not support sample rate " << audioCodecCtx->sample_rate;
		exit(1);
	}



	/* open it */
	if (avcodec_open2(audioCodecCtx, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	pkt = av_packet_alloc();
	if (!pkt) {
		fprintf(stderr, "could not allocate the packet\n");
		exit(1);
	}

	/* frame containing input raw audio */
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate audio frame\n");
		exit(1);
	}

	frame->nb_samples = audioCodecCtx->frame_size;
	frame->format = audioCodecCtx->sample_fmt;
	frame->channel_layout = audioCodecCtx->channel_layout;

	/* allocate the data buffers */
	int ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate audio data buffers\n");
		exit(1);
	}

#ifdef WRITE_CAPTURE_AAC
	if (!aac_out_file) {
		aac_out_file = fopen("ouput.aac", "wb");
		if (aac_out_file == nullptr)
		{

		}
	}
#endif // WRITE_CAPTURE_AAC

	return 0;
}

int AacEncoder::Encode(const char* src_buf, int src_len, unsigned char* dst_buf)
{
	int planar = av_sample_fmt_is_planar(audioCodecCtx->sample_fmt);
	if (planar)
	{
		// 我编码用的非planer结构
	}
	else
	{
		memcpy(frame->data[0], src_buf, src_len);
	}

	

	int ret;

	/* send the frame for encoding */
	ret = avcodec_send_frame(audioCodecCtx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending the frame to the encoder\n");
		exit(1);
	}

	/* read all the available output packets (in general there may be any
	 * number of them */
	while (ret >= 0) {
		ret = avcodec_receive_packet(audioCodecCtx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return 0;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding audio frame\n");
			exit(1);
		}
#ifdef WRITE_CAPTURE_AAC
		fwrite(pkt->data, 1, pkt->size, aac_out_file);
#endif
		av_packet_unref(pkt);
	}
	return 0;
}

int AacEncoder::StopEncode()
{
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_free_context(&audioCodecCtx);
	return 0;
}
