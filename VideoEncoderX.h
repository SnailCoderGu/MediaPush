#pragma once
#include <stdint.h>
#include "x264.h"

#include "VideoEncode.h"
#define WRITE_CAPTURE_264
class VideoEncoderX : public VideoEncoder
{
public:

	VideoEncoderX();

	int InitEncode(int width, int height, int fps, int bitrate, const char* profile)override;
	unsigned int Encode(unsigned char* src_buf, unsigned char* dst_buf)override;
	int StopEncode()override;

	
private:
	x264_param_t m_tX264Param;
	x264_t* m_pEncoder;
	x264_picture_t m_tPicIn;

	bool m_pIsSupportSlice;
	bool m_bForceKeyFrame;

	long m_bLastBitRateTime;

#ifdef WRITE_CAPTURE_264
	FILE* h264_out_file = nullptr;
#endif // WRITE_CAPTURE_YUV
};

