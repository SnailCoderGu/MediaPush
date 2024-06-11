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
	//while ((codec = av_codec_next(codec))) {
	//	if (codec->encode2 && codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
	//		qDebug() << "Codec Name :" << codec->name;
	//		qDebug() << "Type: " << av_get_media_type_string(codec->type);
	//		qDebug() << "Description: " << (codec->long_name ? codec->long_name : codec->name);
	//		qDebug() << "---";
	//	}
	//}

	codec = avcodec_find_encoder_by_name("libx264");
	//codec = avcodec_find_encoder(AVCodecID::AV_CODEC_ID_H264);


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

	//决定了是否在每个i帧前面带sps，pps，如果全局head ，videoCodecCtx->extradata里面就会带sps，pps
	videoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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


	memset(frame->data[0], 0, videoCodecCtx->height * frame->linesize[0]);  // Y
	memset(frame->data[1], 0, videoCodecCtx->height / 2 * frame->linesize[1]); // U
	memset(frame->data[2], 0, videoCodecCtx->height / 2 * frame->linesize[2]); // V

	// 设置全局头部标志
	if (videoCodecCtx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) {
		
	}
	else
	{
		//没有设置全局头，那么就编码一帧数据，获取sps，pps。
		// 发送空帧到编码器以触发 SPS 和 PPS 的生成
		if (avcodec_send_frame(videoCodecCtx, frame) < 0) {
			qDebug() << "Failed to send frame";
			av_frame_free(&frame);
			avcodec_free_context(&videoCodecCtx);
			return -1;
		}


		ret = avcodec_receive_packet(videoCodecCtx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			qDebug() << "Error encoding audio frame " << ret;
			return 0;
		}
		else if (ret < 0) {
			qDebug() << "Error encoding audio frame" << ret;
			exit(1);
		}

#ifdef WRITE_CAPTURE_264
		fwrite(pkt->data, 1, pkt->size, h264_out_file);
#endif // WRITE_CAPTURE_264

		int frame_type = pkt->data[4] & 0x1f;
		if (frame_type == 7 && receive_first_frame)
		{
			//sps,pps,
			CopySpsPps(pkt->data, pkt->size);
			receive_first_frame = false;
		}
	}

	// 保存编码信息，用于外部获取
	codec_config.codec_type = static_cast<int>(AVMEDIA_TYPE_VIDEO);
	codec_config.codec_id = static_cast<int>(AV_CODEC_ID_H264);
	codec_config.width = videoCodecCtx->width;
	codec_config.height = videoCodecCtx->height;
	codec_config.format = videoCodecCtx->pix_fmt;
	codec_config.fps = fps;

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
		memcpy(dst_buf, pkt->data, pkt->size);
	}
	ret = pkt->size;
	av_packet_unref(pkt);


    return ret;
}

void VideoEncodeFF::CopySpsPps(uint8_t* src, int size)
{
	int sps_pps_size = size;
	for (int i = 0;i < size -4;i++)
	{
		if (src[i] == 0 && src[i+1] == 0 && src[i + 2] == 0 && src[i + 3] == 1 && (src[i + 4]&0x1f) == 5)
		{
			sps_pps_size = i + 1;
			break;
		}
		if (src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 1 &&  (src[i + 3] & 0x1f) == 5)
		{
			sps_pps_size = i + 1;
			break;
		}
	}

	videoCodecCtx->extradata = (uint8_t*)av_mallocz(sps_pps_size);
	videoCodecCtx->extradata_size = sps_pps_size;
	memcpy(videoCodecCtx->extradata,src, sps_pps_size);
	return;

	uint8_t* sps_data = nullptr;
	uint8_t* pps_data = nullptr;
	int sps_size = 0;
	int pps_size = 0;
	if (videoCodecCtx->extradata && videoCodecCtx->extradata_size > 0) {
		// 解析 extradata 中的 SPS 和 PPS 数据
		int index = 0;
		if (videoCodecCtx->extradata[index++] == 0x00 && videoCodecCtx->extradata[index++] == 0x00
			&& videoCodecCtx->extradata[index++] == 0x00 && videoCodecCtx->extradata[index++] == 0x01) {
			// 找到了起始码
			int nal_type = videoCodecCtx->extradata[index] & 0x1F;
			if (nal_type == 7) { // SPS
				sps_data = videoCodecCtx->extradata + index;
				sps_size = videoCodecCtx->extradata_size - index;
			}
			else if (nal_type == 8) { // PPS
				pps_data = videoCodecCtx->extradata + index;
				pps_size = videoCodecCtx->extradata_size - index;
			}
		}
	}
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
