#include "AudioCapture.h"
#include <QObject>
#include <QMessageBox>
#include <QPainter>
#include <QtEndian>

RenderArea::RenderArea(QWidget* parent)
    : QWidget(parent)
{
    //setBackgroundRole(QPalette::Base);
    //setAutoFillBackground(true);

    setStyleSheet("background-color: white;");

    setMinimumHeight(30);
    setMinimumWidth(400);
}

void RenderArea::paintEvent(QPaintEvent* /* event */)
{
    QPainter painter(this);

    //painter.setPen(Qt::black);
    painter.drawRect(QRect(painter.viewport().left() + 10,
        painter.viewport().top() + 5,
        painter.viewport().right() - 20,
        painter.viewport().bottom() - 20));

    if (m_level == 0.0)
        return;

    int pos = ((painter.viewport().right() - 20)) * m_level;
    painter.fillRect(painter.viewport().left() + 10,
        painter.viewport().top() + 5,
        pos,
        painter.viewport().height() - 21,
        Qt::red);
}

void RenderArea::setLevel(qreal value)
{
    m_level = value;
    update();
}

/**********************************************************/

AudioCapture::~AudioCapture()
{
    Stop();
}

void AudioCapture::Start(const QAudioDeviceInfo& micInfo)
{
    format = micInfo.preferredFormat();

    switch (format.sampleSize()) {
    case 8:
        switch (format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 255;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 127;
            break;
        default:
            break;
        }
        break;
    case 16:
        switch (format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 65535;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 32767;
            break;
        default:
            break;
        }
        break;

    case 32:
        switch (format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 0xffffffff;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 0x7fffffff;
            break;
        case QAudioFormat::Float:
            m_maxAmplitude = 0x7fffffff; // Kind of
        default:
            break;
        }
        break;

    default:
        break;
    }

    if (!micInfo.isFormatSupported(format)) {
        QMessageBox::warning(nullptr, tr("Audio Capture Error"), "format is not support");
        return;
    }

    qDebug() << "default devicdName: " << micInfo.deviceName() << " : ";
    qDebug() << "sameple: " << format.sampleRate() << " : ";
    qDebug() << "channel:  " << format.channelCount() << " : ";
    qDebug() << "fmt: " << format.sampleSize() << " : ";

    audioInput = new QAudioInput(micInfo, format);

    // 连接errorOccurred信号
    //QObject::connect(audioInput, &this::error,this,&AudioCapture::OnError);

    audioInput->start(this);

    this->open(QIODevice::WriteOnly);

  //  QMessageBox::warning(this, tr("Image Capture Error"), errorString);

}


void AudioCapture::Stop()
{
    if (audioInput)
    {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }

    this->close();
}

void AudioCapture::CaculateLevel(const char* data, qint64 len) {
    if (m_maxAmplitude) {
        
        const int channelBytes = format.sampleSize() / 8;
        const int sampleBytes = format.channelCount() * channelBytes;
        Q_ASSERT(len % sampleBytes == 0);
        const int numSamples = len / sampleBytes;

        quint32 maxValue = 0;
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(data);

        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < format.channelCount(); ++j) {
                quint32 value = 0;

                if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::UnSignedInt) {
                    value = *reinterpret_cast<const quint8*>(ptr);
                }
                else if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::SignedInt) {
                    value = qAbs(*reinterpret_cast<const qint8*>(ptr));
                }
                else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint16>(ptr);
                    else
                        value = qFromBigEndian<quint16>(ptr);
                }
                else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::SignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint16>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint16>(ptr));
                }
                else if (format.sampleSize() == 32 && format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint32>(ptr);
                    else
                        value = qFromBigEndian<quint32>(ptr);
                }
                else if (format.sampleSize() == 32 && format.sampleType() == QAudioFormat::SignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint32>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint32>(ptr));
                }
                else if (format.sampleSize() == 32 && format.sampleType() == QAudioFormat::Float) {
                    value = qAbs(*reinterpret_cast<const float*>(ptr) * 0x7fffffff); // assumes 0-1.0
                }

                maxValue = qMax(value, maxValue);
                ptr += channelBytes;
            }
        }

        maxValue = qMin(maxValue, m_maxAmplitude);
        m_level = qreal(maxValue) / m_maxAmplitude;

        emit updateLevel();
    }


}

qint64 AudioCapture::writeData(const char* data, qint64 len)
{
    // 在这里处理音频数据，例如保存到文件、进行处理等
    qDebug() << "Received audio data. Size:" << len;
    CaculateLevel(data,len);

    emit aframeAvailable(data,len);
    return len;
}
