#include "VideoEncoderX.h"

VideoEncoderX::VideoEncoderX()
{
	m_pEncoder = NULL;
	m_pIsSupportSlice = true;
	m_bForceKeyFrame = false;
	m_bLastBitRateTime = 0;
}

int VideoEncoderX::InitEncode(int width, int height, int fps, int bitrate, const char* profile)
{
	
	if (width <= 0 || height <= 0)
		return -1;

	int nRet = -1;

	char strProfileName[16] = { 0 };
	uint8_t u8Level_idc = 0;

	ParseProfileLevelId(profile, strProfileName, &u8Level_idc);


	nRet = x264_param_default_preset(&m_tX264Param, "ultrafast", "zerolatency");
	if (nRet != 0)
	{
		return -1;
	}

	if (m_pIsSupportSlice)
	{
		m_tX264Param.i_threads = 4; //���벢��ʹ�õ��߳���
		m_tX264Param.b_sliced_threads = 1;
		m_tX264Param.rc.i_lookahead = 0;

		/*************************/
		//m_tX264Param.i_threads = 0;
		//m_tX264Param.b_sliced_threads = 1;
		//m_tX264Param.rc.i_lookahead = 0;
	}
	else
	{
		//ֱ���Ĳ�������һ�㲻slice�ı���
		m_tX264Param.i_threads = 1;
		m_tX264Param.rc.i_lookahead = 0;
	}

	//m_tX264Param.i_log_level = X264_LOG_DEBUG;

	m_tX264Param.i_csp = X264_CSP_I420;

	m_tX264Param.i_width = width;
	m_tX264Param.i_height = height;

	m_tX264Param.i_bframe = 0;

	m_tX264Param.i_fps_num = fps;
	m_tX264Param.i_fps_den = 1;

	m_tX264Param.i_keyint_min = fps * 2;
	m_tX264Param.i_keyint_max = fps * 2;

	m_tX264Param.b_repeat_headers = 1; // �ظ�SPS/PPS �ŵ��ؼ�֡ǰ�� 

	m_tX264Param.i_level_idc = u8Level_idc;

	// x264��̬���ʵ���ֻ������CBR��
	int type = 2;
	switch (type)
	{
	case 0:
		// CQP �㶨qp,��򵥵����ʿ��Ʒ�ʽ��ÿ֡ͼ�񶼰���һ���ض���QP�����룬ÿ֡�������������ж����δ֪�ġ�
		m_tX264Param.rc.i_rc_method = X264_RC_CQP;
		/**
		 ����qp_constant���õ���P֡��QP��I, B֡��QP����f_ip_factor�� f_pb_factor������õ���
		 **/
		m_tX264Param.rc.i_qp_constant = 15;
		m_tX264Param.rc.f_ip_factor = 0.1; //ֵԽС��I֡��qpֵԽ��I֡�Ĵ�СԽС

		break;
	case 1:
		/**********CRF �㶨��������***************/
		m_tX264Param.rc.i_rc_method = X264_RC_CRF;

		m_tX264Param.rc.i_bitrate = bitrate / 1000;
		m_tX264Param.b_annexb = 1;

		m_tX264Param.rc.i_qp_constant = 15;
		//m_tX264Param.rc.i_qp_min     = 15;
		//m_tX264Param.rc.i_qp_max     = 20;


		break;
	case 2:
		/**********CBR �㶨����***************/
		m_tX264Param.rc.i_rc_method = X264_RC_ABR;
		m_tX264Param.b_annexb = 1;
		m_tX264Param.rc.i_qp_min = 15;
		m_tX264Param.rc.i_qp_max = 25;
		m_tX264Param.rc.i_bitrate = bitrate / 1000;
		m_tX264Param.rc.i_vbv_max_bitrate = bitrate / 1000; //ABR���ģʽ�£�˲ʱ��ֵ���ʣ���λkbps,��ֵ��i_bitrate��ȣ�����CBR�㶨���ģʽ��
		m_tX264Param.rc.i_vbv_buffer_size = bitrate / 1000;
		m_tX264Param.rc.f_rate_tolerance = 10; //ABR���ģʽ�£�˲ʱ���ʿ���ƫ��ı���

		m_tX264Param.rc.b_filler = 0; //CBRģʽ�£����ʲ�����ǿ��������λ�������ʡ�

		break;
	case 3:
		/**********ABR ƽ������***************/
		m_tX264Param.rc.i_rc_method = X264_RC_ABR;
		m_tX264Param.b_vfr_input = 0; //��ʱ��fps������timebase, timestamps������֡�����
		m_tX264Param.b_annexb = 1;

		/************************************************************************/
		/* X264_AQ_NONE:������AQģʽ��֡�ں��ȫ��ʹ��ͬһQP���߹̶���QP��
		X264_AQ_VARIANCE:ʹ�÷��̬����ÿ������QP��
		X264_AQ_AUTOVARIANCE:��������Ӧģʽ�����ȱ���һ��ȫ����飬ͳ�Ƴ�һЩ�м������֮��������Щ��������ÿ��������QP��
		X264_AQ_AUTOVARIANCE_BIASED:ƫ�Ʒ�������Ӧģʽ���ڸ�ģʽ��BiasStrength��Ϊԭʼ��Strengthֵ������ÿ������ QP*/
		/************************************************************************/
		//m_tX264Param.rc.i_aq_mode = X264_AQ_VARIANCE;//����Ӧ����������
		m_tX264Param.rc.i_qp_min = 15;
		m_tX264Param.rc.i_qp_max = 25;
		m_tX264Param.rc.i_bitrate = bitrate / 1000;
		m_tX264Param.rc.i_vbv_max_bitrate = bitrate * 1.2 / 1000; //ABR���ģʽ�£�˲ʱ��ֵ���ʣ���λkbps,��ֵ��i_bitrate��ȣ�����CBR�㶨���ģʽ��
		m_tX264Param.rc.i_vbv_buffer_size = bitrate / 1000 * 2; //���ʿ��ƻ������Ĵ�С����λkbit,��Ӱ���Ӿ磬������������3��i_vbv_max_bitrate��
		m_tX264Param.rc.f_rate_tolerance = 1.5; //ABR���ģʽ�£�˲ʱ���ʿ���ƫ��ı���

		//m_tX264Param.rc.f_ip_factor = 0.1; //ֵԽС��I֡��qpֵԼ��I֡�Ĵ�СԽС
		break;
	}


	nRet = x264_param_apply_profile(&m_tX264Param, strProfileName);
	if (nRet != 0)
	{
		return -1;
	}

	x264_picture_init(&m_tPicIn);
	m_tPicIn.img.i_csp = m_tX264Param.i_csp;
	m_tPicIn.img.i_plane = 3;
	m_tPicIn.img.i_stride[0] = m_tX264Param.i_width;
	m_tPicIn.img.i_stride[1] = m_tX264Param.i_width / 2;
	m_tPicIn.img.i_stride[2] = m_tX264Param.i_width / 2;

	//nRet = x264_picture_alloc(&m_tPicIn, m_tX264Param.i_csp, m_tX264Param.i_width, m_tX264Param.i_height);  
	//if(nRet != 0)
	//{
	//	printf("pic alloc failed errno: %d\n", nRet);
	//	return -1;
	//}	

	m_pEncoder = x264_encoder_open(&m_tX264Param);
	if (!m_pEncoder)
	{
		x264_encoder_close(m_pEncoder);
		x264_picture_clean(&m_tPicIn);
		m_pEncoder = NULL;

		return -1;
	}
#ifdef WRITE_CAPTURE_264
	if (!h264_out_file) {
		h264_out_file = fopen("ouput.264", "wb");
		if (h264_out_file == nullptr)
		{
			
		}
	}
#endif // WRITE_CAPTURE_264
	

	return 0;
}

unsigned int VideoEncoderX::Encode(unsigned char* src_buf, unsigned char* dst_buf)
{
	if (!src_buf || !m_pEncoder)
	{
		return -1;
	}

	unsigned int nRet = 0;
	x264_nal_t* nal;
	int i_nal = 0;
	x264_picture_t tPicOut;

	int y_size = m_tX264Param.i_width * m_tX264Param.i_height;
	m_tPicIn.img.plane[0] = src_buf;
	m_tPicIn.img.plane[1] = src_buf + y_size;
	m_tPicIn.img.plane[2] = src_buf + y_size * 5 / 4;
	if (m_bForceKeyFrame)
	{
		m_tPicIn.i_type = X264_TYPE_KEYFRAME;
	}
	else
	{
		m_tPicIn.i_type = X264_TYPE_AUTO;
	}
	m_bForceKeyFrame = false;


	int i_frame_size = x264_encoder_encode(m_pEncoder, &nal, &i_nal, &m_tPicIn, &tPicOut);

	if (i_frame_size)
	{
		for (int i = 0; i < i_nal; i++)
		{
			if (dst_buf) {
				memcpy(dst_buf + nRet, nal[i].p_payload, nal[i].i_payload);
			}
			//memcpy(dst_buf, nal[i].p_payload, nal[i].i_payload);
			//dst_buf += nal[i].i_payload;

#ifdef WRITE_CAPTURE_264
			fwrite(nal[i].p_payload, 1, nal[i].i_payload, h264_out_file);
#endif // WRITE_CAPTURE_264

			nRet += nal[i].i_payload;
		}
	}
	if (tPicOut.b_keyframe)
	{
		//LOGGER_DEBUG << "keyframe:" << tPicOut.b_keyframe << " type: " << tPicOut.i_type << " frame size: " << nRet << " byte";
	}

	return nRet;
}

int VideoEncoderX::StopEncode()
{

	if (m_pEncoder)
	{
		x264_encoder_close(m_pEncoder);
		m_pEncoder = NULL;

	}
	return 0;

}
