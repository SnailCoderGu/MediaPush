#pragma once
/*
* 使用librtmp库做rtmp推流
*/
#pragma comment(lib, "ws2_32.lib")

#include "rtmp.h"
#include "log.h"

#include <string>
#include <iostream>
#include <chrono>
#include <mutex>

namespace live
{

	struct h264_nal
	{
		char	 type;
		unsigned char* pdata;		// not included 00 00 00 01.. head flag
		int		 len;
	};

	enum FrameType
	{

		VIDEO,
		AUDIO
	};

	//enum GP_NAL_TYPE
	//{
	//	NAL_SPS = 7,
	//	NAL_PPS = 8,
	//	NAL_SLICE_IDR = 5,
	//	NAL_SLICE = 1,
	//};

	enum
	{
		FLV_CODECID_H264 = 7,
	};



	struct RTMPMetadata
	{
		// video, must be h264 type  
		unsigned int    nWidth;
		unsigned int    nHeight;
		unsigned int    nFrameRate;     // fps  
		unsigned int    nVideoDataRate; // bps  
		unsigned int    nSpsLen;
		unsigned char   Sps[1024];
		unsigned int    nPpsLen;
		unsigned char   Pps[1024];

		// audio, must be aac type  
		bool            bHasAudio;
		unsigned int    nAudioSampleRate;
		unsigned int    nAudioSampleSize;
		unsigned int    nAudioChannels;
		char            pAudioSpecCfg;
		unsigned int    nAudioSpecCfgLen;

	};

	class RtmpPushLibrtmp
	{
	public:
		RtmpPushLibrtmp();
		int Init(std::string url, 
			int width,
			int height,
			int fps,
			int sample_rate,
			int channel);
		void Close();

		int SendAudio(unsigned char* buf, int len);
		int SendVideo(unsigned char* buf, int len);
		int SendSpsPps(const uint8_t* extradata,
			int extextradata_size);

		int SendAacSpec(const uint8_t* extradata,
			int extextradata_size);



	private:
		
		int isNalTail(const uint8_t* lpData, int size)
		{
			int i = 0;
			if (size >= 4 && lpData[i] == 0 && lpData[i + 1] == 0 && lpData[i + 2] == 0 && lpData[i + 3] == 1)
				return 4;

			if (size >= 3 && lpData[i] == 0 && lpData[i + 1] == 0 && lpData[i + 2] == 1)
				return 3;

			if (size >= 3 && lpData[i] == 0 && lpData[i + 1] == 0 && lpData[i + 2] == 0)
				return 3;

			return 0;
		}

		
		
		

	private:
		bool SendMetadata(RTMPMetadata& lpMetaData);
	private:
		RTMP* m_pRtmp;

		// video info
		int width_;
		int height_;
		int fps_;

		//audo_info
		int sample_rate_;
		int channel_;


		int64_t start_time;

		std::mutex librmtp_mutex;


	};

}

