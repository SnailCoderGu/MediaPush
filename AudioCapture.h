#pragma once
#include <QAudioInput>
#include <QIODevice>
#include <QAudio>
#include <stdio.h>
#include <QDebug>
#include <QWidget>
#include <QPaintEvent>
#include "SwrResample.h"

#define WRITE_RAW_PCM_FILE

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    explicit RenderArea(QWidget* parent = nullptr);

    void setLevel(qreal value);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qreal m_level = 0;
    QPixmap m_pixmap;
};

struct TFormat {
    int sample_rate;
    int chanel_layout;
    AVSampleFormat sample_fmt;
};

class AudioCapture : public QIODevice
{
    Q_OBJECT
public:
	AudioCapture()
	{
      
        //QAudioFormat format;
        //format.setSampleRate(44100); // 采样率
        //format.setChannelCount(2);   // 单声道
        //format.setSampleSize(16);    // 采样大小
        //format.setCodec("audio/pcm");
        //format.setByteOrder(QAudioFormat::LittleEndian);
        //format.setSampleType(QAudioFormat::SignedInt); 
	}
    ~AudioCapture();

public:

    inline void OpenWrite() { write_flag = true; }
    inline void CloseWrite() { write_flag = false; }
    void Start(const QAudioDeviceInfo& micInfo);

    void Stop();

    qint64 readData(char* data, qint64 maxlen) override
    {
        Q_UNUSED(data)
            Q_UNUSED(maxlen)
            return 0; // 不实际从设备中读取数据，因为我们处理的是输入数据
    }

    qint64 writeData(const char* data, qint64 len) override;

    void CaculateLevel(const char* data, qint64 len);

    // 采集器输出的数据，按照1024个采样的频率输入数据，去重采样
    void PaceToResample(const char* data, qint64 len);

	// 重采样输出的数据，按照1024个采样的频率输入数据，去重采样
	void PaceToEncode(char* data, qint64 len);

    void InitDecDataSize(int len);


    qreal level() const { return m_level; }

    TFormat& format()
    {
        return dst_format;
    }

signals:
    void aframeAvailable(char* data, qint64 len);
    void updateLevel();

private:
    QAudioInput* audioInput = nullptr;

    QAudioFormat m_pFormat;
    quint32 m_maxAmplitude = 0;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0

    SwrResample* m_pSwr = nullptr;

    TFormat dst_format;

    const int nb_sample = 1024;  // 取1024个采样，主要是方便后面的aac编码
    int nb_sample_size = 0; //nb_sample个采样对应的字节数，要计算

    int dst_nb_sample_size = 0; //目标采样的数据个数

    //重组给重采样的
    char* src_swr_data =  nullptr;
    int nb_swr_remain = 0;

    //重采样后数据再重组给编码器的
    char* dst_enc_data = nullptr;
    int dst_enc_nb_sample_size = 0;
    int nb_enc_remain = 0;

    bool write_flag = false;
#ifdef WRITE_RAW_PCM_FILE
    FILE* out_raw_pcm_file;
#endif

};

