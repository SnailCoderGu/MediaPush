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

    qreal level() const { return m_level; }

    TFormat& format()
    {
        return dst_format;
    }

signals:
    void aframeAvailable(const char* data, qint64 len);
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

    char* src_swr_data =  nullptr;
    int nb_swr_remain = 0;

    char* dst_swr_data = nullptr;

    bool write_flag = false;
#ifdef WRITE_RAW_PCM_FILE
    FILE* out_raw_pcm_file;
#endif

};

