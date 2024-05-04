#pragma once
#include <QAudioInput>
#include <QIODevice>
#include <QAudio>
#include <stdio.h>
#include <QDebug>
#include <QWidget>
#include <QPaintEvent>

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

class AudioCapture : public QIODevice
{
    Q_OBJECT
public:
	AudioCapture()
	{
      
        //QAudioFormat format;
        //format.setSampleRate(44100); // ������
        //format.setChannelCount(2);   // ������
        //format.setSampleSize(16);    // ������С
        //format.setCodec("audio/pcm");
        //format.setByteOrder(QAudioFormat::LittleEndian);
        //format.setSampleType(QAudioFormat::SignedInt); 
	}
    ~AudioCapture();

public:

    void Start(const QAudioDeviceInfo& micInfo);

    void Stop();

    qint64 readData(char* data, qint64 maxlen) override
    {
        Q_UNUSED(data)
            Q_UNUSED(maxlen)
            return 0; // ��ʵ�ʴ��豸�ж�ȡ���ݣ���Ϊ���Ǵ��������������
    }

    qint64 writeData(const char* data, qint64 len);

    void CaculateLevel(const char* data, qint64 len);

    qreal level() const { return m_level; }

signals:
    void aframeAvailable(const char* data, qint64 len);
    void updateLevel();

private:
    QAudioInput* audioInput = nullptr;

    QAudioFormat format;
    quint32 m_maxAmplitude = 0;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0

};

