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

    //QAudioFormat target_format;
    //target_format.setSampleRate(48000);
    //target_format.setChannelCount(2);

    //m_pFormat = micInfo.nearestFormat(target_format);

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
    int dst_rate = 44100;

    AVSampleFormat src_sample_fmt = sample_fmt;
    AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_S16;

    dst_format.chanel_layout = dst_ch_layout;
    dst_format.sample_fmt = dst_sample_fmt;
    dst_format.sample_rate = dst_rate;

    nb_sample_size = m_pFormat.channelCount() * (m_pFormat.sampleSize() / 8) * nb_sample;
    src_swr_data = new char[nb_sample_size];

    m_pSwr->Init(src_ch_layout, dst_ch_layout, src_rate, dst_rate, src_sample_fmt, dst_sample_fmt, nb_sample);

    audioInput = new QAudioInput(micInfo, m_pFormat);
    //int64_t cbuffSize = audioInput->bufferSize();
    //int64_t bufferSize = m_pFormat.bytesForDuration(1000*1000); // 将 10 毫秒转换为字节数
    //audioInput->setBufferSize(bufferSize);
    
    audioInput->start(this);

    this->open(QIODevice::WriteOnly);

}

void AudioCapture::InitDecDataSize(int len)
{
    if (dst_enc_data)
    {
        delete[] dst_enc_data;
        dst_enc_data = nullptr;
        dst_enc_nb_sample_size = 0;
    }
    dst_enc_nb_sample_size = len;
    dst_enc_data = new char[dst_enc_nb_sample_size];
}


void AudioCapture::Stop()
{
    if (audioInput)
    {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
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

	if (dst_enc_data)
	{
		delete[] dst_enc_data;
		dst_enc_data = nullptr;
		dst_enc_nb_sample_size = 0;
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

void AudioCapture::PaceToResample(const char* data, qint64 len)
{

#ifdef WRITE_RAW_PCM_FILE
	fwrite(data, 1, len, out_raw_pcm_file);
#endif

    if (len != nb_sample_size)
    {
        qDebug() << "Reorganize resample dst data len is wrong " << len;
        return;
    }

	//消费数据
   
	m_pSwr->WriteInput(src_swr_data, nb_sample_size);   //写如数据
     char* result_data = nullptr;
	int rlen = m_pSwr->SwrConvert(&result_data); //执行重采样

    if (dst_enc_data)
    {
        PaceToEncode(result_data, rlen);
    }
   
	
}

void AudioCapture::PaceToEncode(char* data, qint64 len)
{

	//再重行组织一次1024长度给编码器
	int pre_remian = nb_enc_remain + len;  //预计的当前拥有的数据总长度
	char* data_curr = data;
	int len_curr = len;
	while (true) {
		if (pre_remian < dst_enc_nb_sample_size)
		{
			memcpy(dst_enc_data + nb_enc_remain, data_curr, len);
            nb_enc_remain = pre_remian;
			break;
		}
		else
		{
			int missing_len = dst_enc_nb_sample_size - nb_enc_remain;
			memcpy(dst_enc_data + nb_enc_remain, data_curr, missing_len);
            
			emit aframeAvailable(dst_enc_data, dst_enc_nb_sample_size);

            nb_enc_remain = 0;

			pre_remian = pre_remian - dst_enc_nb_sample_size; //消费了一个单位
			data_curr = data_curr + missing_len;
			len_curr = len - missing_len;

		}
	}

	
}


qint64 AudioCapture::writeData(const char* data, qint64 len)
{
    // 在这里处理音频数据，例如保存到文件、进行处理等
    //qDebug() << "Received audio data. Size:" << len;
    CaculateLevel(data,len);

    if (write_flag)
    {

        int pre_remian = nb_swr_remain + len;  //预计的当前拥有的数据总长度
        char* data_curr = const_cast<char*>(data);
        int len_curr = len;
        while (true){
            if (pre_remian < nb_sample_size)
            {
                memcpy(src_swr_data + nb_swr_remain, data_curr, len);
                nb_swr_remain = pre_remian;
                break;
            }
            else
            {
                int missing_len = nb_sample_size - nb_swr_remain;
                memcpy(src_swr_data + nb_swr_remain, data_curr, missing_len);

                PaceToResample(src_swr_data, nb_sample_size);
                nb_swr_remain = 0;

                pre_remian = pre_remian - nb_sample_size; //消费了一个单位
                data_curr = data_curr + missing_len;
                len_curr = len - missing_len;

            }
        }
        
    }

    return len;
}
