#pragma once
#include <string>

#define H264_PROFILE_BASELINE		66
#define H264_PROFILE_MAIN			77
#define H264_PROFILE_HIGH			100

//int VIDEO_ENCODE_MODE = 3;

class VideoEncoder
{
public:
	struct VCodecConfig
	{
		int codec_type;
		int codec_id;
		int width;
		int height;
		int fps;
		int format;
	};

public:
	virtual ~VideoEncoder() {};
	VideoEncoder() {};
	virtual int InitEncode(int width, int height, int fps, int bitrate, const char* profile) = 0;
	virtual unsigned int Encode(unsigned char* src_buf, unsigned char* dst_buf) = 0;
	virtual int StopEncode() = 0;

	inline virtual VCodecConfig& GetCodecConfig() { return codec_config; }


protected:
	void ParseProfileLevelId(const char* profile, char* profilename, uint8_t* levelidc)
	{
		if (!profile || !profilename || !levelidc)
		{
			return;
		}

		char buffer[3];
		uint8_t profile_idc;
		uint8_t profile_iop;
		uint8_t level_idc;

		buffer[0] = profile[0];
		buffer[1] = profile[1];
		buffer[2] = '\0';
		profile_idc = strtol(buffer, NULL, 16);
		buffer[0] = profile[2];
		buffer[1] = profile[3];
		profile_iop = strtol(buffer, NULL, 16);
		buffer[0] = profile[4];
		buffer[1] = profile[5];
		level_idc = strtol(buffer, NULL, 16);

		if (profile_idc == H264_PROFILE_BASELINE)
		{
			strcpy(profilename, "baseline");
		}
		else if (profile_idc == H264_PROFILE_MAIN)
		{
			strcpy(profilename, "main");
		}
		else if (profile_idc == H264_PROFILE_HIGH)
		{
			strcpy(profilename, "high");
		}
		else
		{
			printf("unknown profile\n");
		}

		*levelidc = level_idc;
	}

	VCodecConfig codec_config;


};
