#include "SwrResample.h"

int  SwrResample::Init(int64_t src_ch_layout, int64_t dst_ch_layout,
    int src_rate, int dst_rate,
    enum AVSampleFormat src_sample_fmt, enum AVSampleFormat dst_sample_fmt,
    int src_nb_samples)
{
#ifdef WRITE_RESAMPLE_PCM_FILE
    out_resample_pcm_file = fopen("capture_resample.pcm", "wb");
    if (!out_resample_pcm_file) {
        std::cout << "open out put swr file failed";
    }
#endif // WRITE_RESAMPLE_PCM_FILE

    src_sample_fmt_ = src_sample_fmt;
    dst_sample_fmt_ = dst_sample_fmt;

    src_rate_ = src_rate;
    dst_rate_ = dst_rate;

    int ret;
    /* create resampler context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        std::cout << "Could not allocate resampler context" << std::endl;
        ret = AVERROR(ENOMEM);
        return ret;
    }

    /* set options */
    av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(swr_ctx)) < 0) {
        std::cout << "Failed to initialize the resampling context" << std::endl;
        return -1;
    }

    //配置输入的参数
    /*
    * src_nb_samples: 描述一整的采样个数 比如这里就是 1024
    * src_linesize: 描述一行采样字节长度 
    *   当是planr 结构 LLLLLRRRRRR 的时候 比如 一帧1024个采样，32位表示。那就是 1024*4 = 4096 
    *   当是非palner 结构的时候 LRLRLR 比如一帧1024采样 32位表示 双通道   1024*4*2 = 8196 要乘以通道
    * src_nb_channels : 可以根据布局获得音频的通道
    * ret 返回输入数据的长度 比如这里 1024 * 4 * 2 = 8196 (32bit，双声道，1024个采样)
    */
    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    
    ret = av_samples_alloc_array_and_samples(&src_data_, &src_linesize, src_nb_channels,
        src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        std::cout << "Could not allocate source samples\n" << std::endl;
        return -1;
    }
    src_nb_samples_ = src_nb_samples;

    //配置输出的参数
    max_dst_nb_samples = dst_nb_samples_ =
        av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    
    ret = av_samples_alloc_array_and_samples(&dst_data_, &dst_linesize, dst_nb_channels,
        dst_nb_samples_, dst_sample_fmt, 0);
    if (ret < 0) {
        std::cout << "Could not allocate destination samples" << std::endl;
        return -1;
    }

    int data_size = av_get_bytes_per_sample(dst_sample_fmt_);
    
    return 0;
}

int SwrResample::WriteInput(const char* data, uint64_t len)
{
    int planar = av_sample_fmt_is_planar(src_sample_fmt_);
    int data_size = av_get_bytes_per_sample(src_sample_fmt_);

	if (planar)
	{
		//这里只需要考虑非planaer结构，采集输出的是LRLRLR结构的
	}
	else
	{
        memcpy(src_data_[0], data, len);
	}
	return 0;
}

int SwrResample::WriteInput(AVFrame* frame)
{
    int planar = av_sample_fmt_is_planar(src_sample_fmt_);
    int data_size = av_get_bytes_per_sample(src_sample_fmt_);
    if (planar)
    {
        //src是planar类型的话，src_data里面数据是LLLLLLLRRRRR 结构，src_data_[0] 指向全部的L，src_data_[1] 指向全部R
        // src_data_ 里面其实一个长 uint8_t *buf，src_data_[0] 指向L开始的位置，src_data_[1]指向R的位置
        // linesize 是 b_samples * sample_size 就是比如 48000*4
        for (int ch = 0; ch < src_nb_channels; ch++) {
            memcpy(src_data_[ch], frame->data[ch], data_size * frame->nb_samples);
        }
    }
    else
    {
        //src是非planar类型的话，src_data里面数据是LRLRLRLR 结构，src_data_[0] 指向全部数据 没有src_data[1]
        // linesize 是nb_samples * sample_size * nb_channels 比如 48000*4*2
        for (int i = 0; i < frame->nb_samples; i++){
            for (int ch = 0; ch < src_nb_channels; ch++)
            {
                memcpy(src_data_[0], frame->data[ch]+data_size*i, data_size);
            }
        }
    }
    return 0;
}


int SwrResample::SwrConvert(char** data)
{
    int ret = 0;

    //误差排查
    dst_nb_samples_ = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate_) +
        src_nb_samples_, dst_rate_, src_rate_, AV_ROUND_UP);

	if (dst_nb_samples_ > max_dst_nb_samples) {
		av_freep(&dst_data_[0]);
		ret = av_samples_alloc(dst_data_, &dst_linesize, dst_nb_channels,
            dst_nb_samples_, dst_sample_fmt_, 1);
        if (ret < 0)
            return -1;
		max_dst_nb_samples = dst_nb_samples_;
	}

    ret = swr_convert(swr_ctx, dst_data_, dst_nb_samples_, (const uint8_t**)src_data_, src_nb_samples_);
    if (ret < 0) {
        fprintf(stderr, "Error while converting\n");
        exit(1);
    }
    // ret 返回的长度理论和dst_nb_samples_一致，但是实际不是，这也是上面产生误差的原因

    int  dst_bufsize = av_samples_get_buffer_size(nullptr, dst_nb_channels,
        ret, dst_sample_fmt_, 1);

   int planar = av_sample_fmt_is_planar(dst_sample_fmt_);
   if (planar)
   {
       //目标只用非planer结构，因为我手动控制了输出的fmt，方便逻辑简单
   }
   else {
       //非planr结构，dst_data_[0] 里面存在着全部数据
#ifdef WRITE_RESAMPLE_PCM_FILE
       fwrite(dst_data_[0], 1, dst_bufsize, out_resample_pcm_file);
#endif
       
       *data = (char*)(dst_data_[0]); //这里用二级指针，没有拷贝，是因为考虑到实际重采样的长度，不是很标准
       
   }

    return dst_bufsize;
}

void SwrResample::Close()
{

#ifdef WRITE_RESAMPLE_PCM_FILE
    fclose(out_resample_pcm_file);
#endif

    if (src_data_)
        av_freep(&src_data_[0]);

    av_freep(&src_data_);

    if (dst_data_)
        av_freep(&dst_data_[0]);

    av_freep(&dst_data_);

    swr_free(&swr_ctx);
}
