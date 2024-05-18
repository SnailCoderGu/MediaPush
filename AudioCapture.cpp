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

#ifdef WRITE_RAW_PCM_FILE
    out_raw_pcm_file = fopen("capture_raw.pcm", "wb");
    if (!out_raw_pcm_file) {
        std::cout << "open out put raw file failed";
    }
#endif // WRITE_RESAMPLE_PCM_FILE

    m_pFormat = micInfo.preferredFormat();

    AVSampleFormat sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;

    switch (m_pFormat.sampleSize()) {
    case 8:
        switch (m_pFormat.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 255;
            sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_U8;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 127;
            sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_U8;
            break;
        default:
            break;
        }
        break;
    case 16:
        switch (m_pFormat.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 65535;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 32767;
            sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;
            break;
        default:
            break;
        }
        break;

    case 32:
        switch (m_pFormat.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 0xffffffff;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 0x7fffffff;
            sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S32;
            break;
        case QAudioFormat::Float:
            m_maxAmplitude = 0x7fffffff; // Kind of
            sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_FLT;
        default:
            break;
        }
        break;

    default:
        break;
    }

    if (!micInfo.isFormatSupported(m_pFormat)) {
        QMessageBox::warning(nullptr, tr("Audio Capture Error"), "format is not support");
        return;
    }

    qDebug() << "default devicdName: " << micInfo.deviceName() << " : ";
    qDebug() << "sameple: " << m_pFormat.sampleRate() << " : ";
    qDebug() << "channel:  " << m_pFormat.channelCount() << " : ";
    qDebug() << "fmt: " << m_pFormat.sampleSize() << " : ";
    qDebug() << "bytesPerFrame" << m_pFormat.bytesPerFrame();

    

    m_pSwr = new SwrResample();

    //目标和源采样数据格式
    int channel_count= m_pFormat.channelCount();
    int64_t src_ch_layout = (channel_count == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO);
    int64_t dst_ch_layout = src_ch_layout;

    int src_rate = m_pFormat.sampleRate();
    int dst_rate = src_rate;

    AVSampleFormat src_sample_fmt = sample_fmt;
    AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

    dst_format.chanel_layout = dst_ch_layout;
    dst_format.sample_fmt = dst_sample_fmt;
    dst_format.sample_rate = dst_rate;

    nb_sample_size = m_pFormat.channelCount() * (m_pFormat.sampleSize() / 8) * nb_sample;
    src_swr_data = new char[nb_sample_size];

    m_pSwr->Init(src_ch_layout, dst_ch_layout, src_rate, dst_rate, src_sample_fmt, dst_sample_fmt, nb_sample);

    int dst_nb_sample_size = m_pSwr->GetDstNbSample() * (m_pFormat.sampleSize() / 8) * channel_count;
    dst_swr_data = new char[dst_nb_sample_size];

    audioInput = new QAudioInput(micInfo, m_pFormat);
    //int64_t cbuffSize = audioInput->bufferSize();
    //int64_t bufferSize = m_pFormat.bytesForDuration(1000*1000); // 将 10 毫秒转换为字节数
    //audioInput->setBufferSize(bufferSize);
    
    audioInput->start(this);

    this->open(QIODevice::WriteOnly);

}


void AudioCapture::Stop()
{


    if (audioInput)
    {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }

    if (dst_swr_data)
    {
        delete[] dst_swr_data;
        dst_swr_data = nullptr;
    }

    if (m_pSwr)
    {
        m_pSwr->Close();
        delete m_pSwr;
        m_pSwr = nullptr;
    }

    if (src_swr_data)
    {
        delete[] src_swr_data;
        src_swr_data = nullptr;
    }

    this->close();

#ifdef WRITE_RAW_PCM_FILE
    fclose(out_raw_pcm_file);
#endif
}

void AudioCapture::CaculateLevel(const char* data, qint64 len) {
    if (m_maxAmplitude) {
        
        const int channelBytes = m_pFormat.sampleSize() / 8;
        const int sampleBytes = m_pFormat.channelCount() * channelBytes;
        Q_ASSERT(len % sampleBytes == 0);
        const int numSamples = len / sampleBytes;

        quint32 maxValue = 0;
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(data);

        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < m_pFormat.channelCount(); ++j) {
                quint32 value = 0;

                if (m_pFormat.sampleSize() == 8 && m_pFormat.sampleType() == QAudioFormat::UnSignedInt) {
                    value = *reinterpret_cast<const quint8*>(ptr);
                }
                else if (m_pFormat.sampleSize() == 8 && m_pFormat.sampleType() == QAudioFormat::SignedInt) {
                    value = qAbs(*reinterpret_cast<const qint8*>(ptr));
                }
                else if (m_pFormat.sampleSize() == 16 && m_pFormat.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_pFormat.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint16>(ptr);
                    else
                        value = qFromBigEndian<quint16>(ptr);
                }
                else if (m_pFormat.sampleSize() == 16 && m_pFormat.sampleType() == QAudioFormat::SignedInt) {
                    if (m_pFormat.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint16>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint16>(ptr));
                }
                else if (m_pFormat.sampleSize() == 32 && m_pFormat.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_pFormat.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint32>(ptr);
                    else
                        value = qFromBigEndian<quint32>(ptr);
                }
                else if (m_pFormat.sampleSize() == 32 && m_pFormat.sampleType() == QAudioFormat::SignedInt) {
                    if (m_pFormat.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint32>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint32>(ptr));
                }
                else if (m_pFormat.sampleSize() == 32 && m_pFormat.sampleType() == QAudioFormat::Float) {
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
    //qDebug() << "Received audio data. Size:" << len;
    CaculateLevel(data,len);


    
    if (write_flag)
    {
        if (nb_swr_remain + len < nb_sample_size)
        {
            memcpy(src_swr_data + nb_swr_remain, data, len);
            nb_swr_remain += len;
        }
        else
        {
            int out_size = nb_swr_remain + len - nb_sample_size;
            memcpy(src_swr_data+ nb_swr_remain, data, len - out_size);

#ifdef WRITE_RAW_PCM_FILE
            fwrite(src_swr_data, 1, nb_sample_size, out_raw_pcm_file);
#endif

            //消费数据
            m_pSwr->WriteInput(src_swr_data, nb_sample_size);
            int rlen = m_pSwr->SwrConvert(dst_swr_data);
            emit aframeAvailable(dst_swr_data, rlen);

            //重新取一个开始
            nb_swr_remain = out_size;
            if (out_size > 0)
            {
                  memcpy(src_swr_data, data + (len- out_size), out_size);
            }
        }
        
    }

    return len;
}
