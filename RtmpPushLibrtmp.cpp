#include "RtmpPushLibrtmp.h"
#include <stdio.h>
#include <winsock2.h>
namespace live
{

	char* put_byte(char* output, uint8_t nVal)
	{
		output[0] = nVal;
		return output + 1;
	}
	char* put_be16(char* output, uint16_t nVal)
	{
		output[1] = nVal & 0xff;
		output[0] = nVal >> 8;
		return output + 2;
	}
	char* put_be24(char* output, uint32_t nVal)
	{
		output[2] = nVal & 0xff;
		output[1] = nVal >> 8;
		output[0] = nVal >> 16;
		return output + 3;
	}
	char* put_be32(char* output, uint32_t nVal)
	{
		output[3] = nVal & 0xff;
		output[2] = nVal >> 8;
		output[1] = nVal >> 16;
		output[0] = nVal >> 24;
		return output + 4;
	}
	char* put_be64(char* output, uint64_t nVal)
	{
		output = put_be32(output, nVal >> 32);
		output = put_be32(output, nVal);
		return output;
	}
	char* put_amf_string(char* c, const char* str)
	{
		uint16_t len = strlen(str);
		c = put_be16(c, len);
		memcpy(c, str, len);
		return c + len;
	}
	char* put_amf_double(char* c, double d)
	{
		*c++ = AMF_NUMBER;  /* type: Number */
		{
			unsigned char* ci, * co;
			ci = (unsigned char*)&d;
			co = (unsigned char*)c;
			co[0] = ci[7];
			co[1] = ci[6];
			co[2] = ci[5];
			co[3] = ci[4];
			co[4] = ci[3];
			co[5] = ci[2];
			co[6] = ci[1];
			co[7] = ci[0];
		}
		return c + 8;
	}


	RtmpPushLibrtmp::RtmpPushLibrtmp()
	{
		//创建一个RTMP会话的句柄
		m_pRtmp = RTMP_Alloc();

		//初始化句柄
		RTMP_Init(m_pRtmp);

		m_pRtmp->Link.timeout = 30;

		//RTMP_LogLevel lvl = RTMP_LogLevel::RTMP_LOGALL;
		RTMP_LogLevel lvl = RTMP_LogLevel::RTMP_LOGERROR;
		RTMP_LogSetLevel(lvl);
	}

	int RtmpPushLibrtmp::Init(std::string url,int width,
		int height,
		int fps,
		int sample_rate,
		int channel) 
	{
		width_ = width;
		height_ = height;
		fps_ = fps;
		sample_rate_ = sample_rate;
		channel_ = channel;

		if (!RTMP_SetupURL(m_pRtmp, (char*)url.c_str()))
		{
			std::cout << "rtmp setupurl faild: " << std::endl;
			return -1;
		}

		RTMP_EnableWrite(m_pRtmp);


		/////// window 环境需要的
		WSADATA wsaData;
		int nRet;
		if ((nRet = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
			return nRet;
		}
		////////////////////

		if (!RTMP_Connect(m_pRtmp, NULL))
		{
			std::cout << "rtmp connect faild: " << url << std::endl;
			return -1;
		}
		if (!RTMP_ConnectStream(m_pRtmp, 0))
		{
			std::cout << "rtmp connect stream faild: " << url << std::endl;
			return -2;
		}

		RTMPMetadata metadta;
		metadta.nWidth = width_;
		metadta.nHeight = height_;
		metadta.nFrameRate = fps_;
		metadta.nAudioSampleRate = sample_rate_;
		metadta.nAudioChannels = channel_;
		metadta.bHasAudio = true;

		SendMetadata(metadta);
		

		auto now = std::chrono::steady_clock::now();
		start_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();


	}

	bool RtmpPushLibrtmp::SendMetadata(RTMPMetadata& lpMetaData)
	{
		char body[1024] = { 0 };

		char* p = (char*)body;
		p = put_byte(p, AMF_STRING);
		p = put_amf_string(p, "@setDataFrame");

		p = put_byte(p, AMF_STRING);
		p = put_amf_string(p, "onMetaData");

		p = put_byte(p, AMF_OBJECT);
		p = put_amf_string(p, "copyright");

		p = put_byte(p, AMF_STRING);
		p = put_amf_string(p, "gupan");

		p = put_amf_string(p, "width");
		p = put_amf_double(p, lpMetaData.nWidth);

		p = put_amf_string(p, "height");
		p = put_amf_double(p, lpMetaData.nHeight);

		p = put_amf_string(p, "framerate");
		p = put_amf_double(p, lpMetaData.nFrameRate);

		p = put_amf_string(p, "videocodecid");
		p = put_amf_double(p, FLV_CODECID_H264);

		p = put_amf_string(p, "audiocodecid");
		p = put_amf_double(p, 10);


		p = put_amf_string(p, "audiodatarate");
		if (lpMetaData.nAudioSampleRate == 44100) {
			p = put_amf_double(p, 64);
		}
		else if (lpMetaData.nAudioSampleRate == 48000)
		{
			p = put_amf_double(p, 512);
		}


		p = put_amf_string(p, "audiochannels");
		p = put_amf_double(p, 2);

		p = put_amf_string(p, "audiosamplesize");
		p = put_amf_double(p, 16);

		p = put_amf_string(p, "audiosamplerate");
		if (lpMetaData.nAudioSampleRate == 44100) {
			p = put_amf_double(p, 44100);
		}
		else if (lpMetaData.nAudioSampleRate == 48000) {
			p = put_amf_double(p, 48000);
		}


		p = put_amf_string(p, "");
		p = put_byte(p, AMF_OBJECT_END);

		int size = p - body;

		RTMPPacket packet;
		RTMPPacket_Reset(&packet);
		RTMPPacket_Alloc(&packet, size);

		packet.m_packetType = RTMP_PACKET_TYPE_INFO;
		packet.m_nChannel = 0x04;
		packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
		packet.m_nTimeStamp = 0;
		packet.m_nInfoField2 = m_pRtmp->m_stream_id;
		packet.m_nBodySize = size;
		memcpy(packet.m_body, body, size);

		int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);

		RTMPPacket_Free(&packet);
		return true;
	}

	int RtmpPushLibrtmp::SendAacSpec(const uint8_t* extradata,
		int extextradata_size)
	{
		RTMPPacket packet;
		RTMPPacket_Reset(&packet);
		RTMPPacket_Alloc(&packet, 2+ extextradata_size);


		packet.m_body[0] = 0xAF;
		packet.m_body[1] = 0x00;

		memcpy(&(packet.m_body[2]), extradata, extextradata_size);

		packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
		packet.m_hasAbsTimestamp = 0;
		packet.m_nChannel = 0x04;
		packet.m_nTimeStamp = 0;
		packet.m_nInfoField2 = m_pRtmp->m_stream_id;
		packet.m_nBodySize = 2+ extextradata_size;

		//调用发送接口  
		int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);
		if (nRet != 1)
		{
			std::cout << "send video error " << nRet << std::endl;
		}
		RTMPPacket_Free(&packet);


		return 1;
	}

	int RtmpPushLibrtmp::SendSpsPps(const uint8_t* extradata,
		int extextradata_size)
	{
		int nal_size = isNalTail(extradata, extextradata_size); //计算nalu头长度，4个字节或者3个字节

		uint8_t* m_pSps = nullptr;
		int m_pSpslen = 0;

		uint8_t* m_pPps = nullptr;
		int m_pPpslen = 0;

		//根据nal头找到pps的位置
		for (int i = 0;i < extextradata_size - 4;i++)
		{
			if (i!=0&&extradata[i] == 0 && extradata[i + 1] == 0 && extradata[i + 2] == 0 && extradata[i + 3] == 1)
			{
				m_pSps = const_cast<uint8_t*>(&extradata[nal_size]);
				m_pSpslen = i + 1 - nal_size;

				m_pPps = const_cast<uint8_t*>(&extradata[i+nal_size]);
				m_pPpslen = extextradata_size - m_pSpslen - nal_size;
				break;
			}
				
		}
		if (m_pSps == nullptr || m_pPps == nullptr)
		{
			std::cout << "parse sps,pps error " << std::endl;
			return -1;
		}

		RTMPPacket packet;
		RTMPPacket_Reset(&packet);
		RTMPPacket_Alloc(&packet, 1024);

		unsigned char* body = (unsigned char*)packet.m_body;

		int i = 0;
		body[i++] = 0x17;
		body[i++] = 0x00;

		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;

		/*AVCDecoderConfigurationRecord，根据sps，设置了一些配置信息*/
		body[i++] = 0x01;
		body[i++] = m_pSps[1];
		body[i++] = m_pSps[2];
		body[i++] = m_pSps[3];
		body[i++] = 0xff;

		/*m_pSps 添加数据*/
		body[i++] = 0xe1;
		body[i++] = (m_pSpslen >> 8) & 0xff;
		body[i++] = m_pSpslen & 0xff;
		memcpy(&body[i], m_pSps, m_pSpslen);
		i += m_pSpslen;

		/*m_pPps 数据添加*/
		body[i++] = 0x01;
		body[i++] = (m_pPpslen >> 8) & 0xff;
		body[i++] = (m_pPpslen) & 0xff;
		memcpy(&body[i], m_pPps, m_pPpslen);
		i += m_pPpslen;

		packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;
		packet.m_nBodySize = i;
		packet.m_nChannel = 0x04;
		packet.m_nTimeStamp = 0;
		packet.m_hasAbsTimestamp = 0;
		packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		packet.m_nInfoField2 = m_pRtmp->m_stream_id;

		int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);  //调用api，发送数据包

		RTMPPacket_Free(&packet);
		return 1;
	}

	int RtmpPushLibrtmp::SendAudio(unsigned char* buf, int len)
	{
		std::lock_guard<std::mutex> lock(librmtp_mutex);

		RTMPPacket packet;
		RTMPPacket_Reset(&packet);
		RTMPPacket_Alloc(&packet, len + 2);
		packet.m_nBodySize = len + 2;

		unsigned char* body = (unsigned char*)packet.m_body;
		memset(body, 0, len + 2);


		/*AF 01 + AAC RAW data*/
		body[0] = 0xAF;
		body[1] = 0x01;
		memcpy(&body[2], buf, len);


		packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
		packet.m_nChannel = 0x04;
		auto now = std::chrono::steady_clock::now();
		int64_t now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		unsigned int timeoffset = now_time - start_time;
		packet.m_nTimeStamp = timeoffset;
		packet.m_hasAbsTimestamp = 0;
		packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		packet.m_nInfoField2 = m_pRtmp->m_stream_id;

		/*调用发送接口*/
		int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);
		if (nRet != 1)
		{
			std::cout << "send audio error " << nRet << std::endl;
		}
		
		RTMPPacket_Free(&packet);

		return 1;
	}

	int RtmpPushLibrtmp::SendVideo(unsigned char* buf, int len)
	{

		std::lock_guard<std::mutex> lock(librmtp_mutex);

		int nal_size = isNalTail(buf, len); //计算nalu头长度，4个字节或者3个字节

		// 去掉nalu头的界定符号
		buf += nal_size;
		len -= nal_size;

		int type = buf[0] & 0x1f;
		
		RTMPPacket packet;
		RTMPPacket_Reset(&packet);
		RTMPPacket_Alloc(&packet, len + 9);

		packet.m_nBodySize = len + 9;

		/*send video packet*/
		unsigned char* body = (unsigned char*)packet.m_body;
		memset(body, 0, len + 9);

		/*key frame*/
		body[0] = 0x27;
		if (type == 5) {
			body[0] = 0x17;
		}
		body[1] = 0x01;   /*nal unit*/
		body[2] = 0x00;
		body[3] = 0x00;
		body[4] = 0x00;

		// raw 数据的长度
		body[5] = (len >> 24) & 0xff;
		body[6] = (len >> 16) & 0xff;
		body[7] = (len >> 8) & 0xff;
		body[8] = (len) & 0xff;

		/*copy data*/
		memcpy(&body[9], buf, len);

		packet.m_hasAbsTimestamp = 0;
		packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;
		packet.m_nInfoField2 = m_pRtmp->m_stream_id;
		packet.m_nChannel = 0x04;
		packet.m_headerType = RTMP_PACKET_SIZE_LARGE;

		auto now = std::chrono::steady_clock::now();
		int64_t now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		unsigned int timeoffset = now_time - start_time;
		packet.m_nTimeStamp = timeoffset;

		/*调用发送接口*/
		int nRet = RTMP_SendPacket(m_pRtmp, &packet, 0);
		if (nRet != 1)
		{
			std::cout << "send video error " << nRet << std::endl;
		}
		
		RTMPPacket_Free(&packet);

		return 1;
	}

	void RtmpPushLibrtmp::Close()
	{
		if (m_pRtmp)
		{
			RTMP_Close(m_pRtmp);
			RTMP_Free(m_pRtmp);
			m_pRtmp = NULL;
		}
	}
}