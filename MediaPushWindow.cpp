#include "MediaPushWindow.h"
#include <QCameraInfo>
#include <QMessageBox>
#include <QPalette>
#include <QDir>
#include <QTimer>
#include <QMediaMetaData>

#include <sstream>


Q_DECLARE_METATYPE(QCameraInfo)

MediaPushWindow::MediaPushWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

	//添加摄像头设备菜单
	QActionGroup* videoDevicesGroup = new QActionGroup(this);
	videoDevicesGroup->setExclusive(true);

	const QList<QCameraInfo> availableCameras = QCameraInfo::availableCameras();
	for (const QCameraInfo& cameraInfo : availableCameras) {
		QAction* videoDeviceAction = new QAction(cameraInfo.description(), videoDevicesGroup);
		videoDeviceAction->setCheckable(true);
		videoDeviceAction->setData(QVariant::fromValue(cameraInfo));
		if (cameraInfo == QCameraInfo::defaultCamera())
			videoDeviceAction->setChecked(true);

		ui.menuVDevices->addAction(videoDeviceAction);
	}
	//监听选择摄像头
	connect(videoDevicesGroup, &QActionGroup::triggered, this, &MediaPushWindow::updateCameraDevice);

	//添加麦克风设备菜单
	QActionGroup* audioDevicesGroup = new QActionGroup(this);
	audioDevicesGroup->setExclusive(true);
	QAudioDeviceInfo default_device_info;
	QList<QAudioDeviceInfo> availableMics =QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput);
	for (const QAudioDeviceInfo& audioInfo : availableMics) {
		QAction* audioDeviceAction = new QAction(audioInfo.deviceName(), audioDevicesGroup);
		audioDeviceAction->setCheckable(true);
		audioDeviceAction->setData(QVariant::fromValue(audioInfo));
		if (audioInfo.deviceName() == QAudioDeviceInfo::defaultInputDevice().deviceName());
		{
			audioDeviceAction->setChecked(true);
			default_device_info = audioInfo;
		}
		

		ui.menuADevices->addAction(audioDeviceAction);
	}
	//监听麦克风设备
	connect(audioDevicesGroup, &QActionGroup::triggered, this, &MediaPushWindow::updateMicDevice);
	
	//视频的模式切换
	connect(ui.captureWidget, &QTabWidget::currentChanged, this, &MediaPushWindow::updateCaptureMode);

	//设置摄像头
	setCamera(QCameraInfo::defaultCamera());
	//设置麦克风
	setMic(default_device_info);

	ui.actionStart->setEnabled(true);
	ui.actionStop->setEnabled(false);
	ui.actionSettings->setEnabled(true);

	renderArea = new RenderArea(this);
	renderArea->setLevel(0.01);
	//renderArea->resize(600, 30);
	ui.statusbar->addWidget(renderArea);

	ui.statusbar->setStyleSheet("background-color: white;");

}

void MediaPushWindow::InitAudioEncode()
{
	int sample_rate = m_mic->format().sample_rate;
	int channel_layout = m_mic->format().chanel_layout;
	AVSampleFormat smaple_fmt = m_mic->format().sample_fmt;
	aacEncoder->InitEncode(sample_rate, 96000, smaple_fmt, channel_layout);

	m_mic->InitDecDataSize(aacEncoder->frame_byte_size);

}

void MediaPushWindow::start()
{
	std::string url = "rtmp://192.168.109.128:1935/live/test";
	ChooseUrlDialog urlDialog;
	if (urlDialog.exec() == QDialog::Accepted) {
		url = urlDialog.url;
	}
	else
	{
		return;
	}
	

	ui.actionStart->setEnabled(false);
	ui.actionStop->setEnabled(true);
	ui.actionSettings->setEnabled(false);

	start_flag = true;

	audio_encoder_data = new unsigned char[1024 * 2 * 2];
	video_encoder_data = new unsigned char[1024 * 1024];

	m_mic->OpenWrite();
	aacEncoder.reset(new AacEncoder());
	InitAudioEncode();


	if (!ffVideoEncoder)
	{
#if USE_FFMPEG_VIDEO_ENCODE
		ffVideoEncoder.reset(new VideoEncodeFF());
		ffVideoEncoder->InitEncode(width_, height_, 25, 2000000, "main");
#else
		ffVideoEncoder.reset(new VideoEncoderX());
		ffVideoEncoder->InitEncode(width_, height_, 25, 2000000, "42801f");
#endif
	}

#ifdef RTMP_PUSH_USER_FFMPEG
	rtmpPush.reset(new RtmpPush());
	rtmpPush->OpenFormat(url);

	
	VideoEncoder::VCodecConfig& vcode_info = ffVideoEncoder->GetCodecConfig();
	rtmpPush->InitVideoCodePar(static_cast<AVMediaType>(vcode_info.codec_type),
		static_cast<AVCodecID>(vcode_info.codec_id),
		vcode_info.width, vcode_info.height, vcode_info.fps, vcode_info.format,
		ffVideoEncoder->GetExterdata(), ffVideoEncoder->GetExterdataSize());

	AudioEncoder::ACodecConfig& acode_info = aacEncoder->GetCodecConfig();
	rtmpPush->InitAudioCodecPar(static_cast<AVMediaType>(acode_info.codec_type),
		static_cast<AVCodecID>(acode_info.codec_id),
		acode_info.sample_rate, acode_info.channel, acode_info.format,
		aacEncoder->GetExterdata(),aacEncoder->GetExterdataSize());

	rtmpPush->WriteHeader();
#else
	rtmpPush.reset(new RtmpPushLibrtmp());
	VideoEncoder::VCodecConfig& vcode_info = ffVideoEncoder->GetCodecConfig();
	AudioEncoder::ACodecConfig& acode_info = aacEncoder->GetCodecConfig();
	rtmpPush->Init(url,
		vcode_info.width, vcode_info.height, vcode_info.fps,
		acode_info.sample_rate, acode_info.channel);

	rtmpPush->SendAacSpec(aacEncoder->GetExterdata(), aacEncoder->GetExterdataSize());
	rtmpPush->SendSpsPps(ffVideoEncoder->GetExterdata(), ffVideoEncoder->GetExterdataSize());


	
#endif

#ifdef WRITE_CAPTURE_YUV
	if (!yuv_out_file) {
		yuv_out_file = fopen("ouput.yuv", "wb");
		if (yuv_out_file == nullptr)
		{
			qDebug() << "Open yuv file failed";
		}
	}

	if (!rgb_out_file) {
		rgb_out_file = fopen("output.rgb", "wb");
		if (rgb_out_file == nullptr)
		{
			qDebug() << "Open rgb file failed";
		}
	}
#endif // WRITE_CAPTURE_YUV

}

void MediaPushWindow::stop()
{
	ui.actionStart->setEnabled(true);
	ui.actionStop->setEnabled(false);
	ui.actionSettings->setEnabled(true);

#ifdef WRITE_CAPTURE_YUV
	if (yuv_out_file) {
		fclose(yuv_out_file);
		yuv_out_file = nullptr;
	}

	if (rgb_out_file) {
		fclose(rgb_out_file);
		rgb_out_file = nullptr;
	}
#endif

	m_mic->CloseWrite();


	if (aacEncoder)
	{
		aacEncoder->StopEncode();
		aacEncoder.reset(nullptr);
	}

	if (ffVideoEncoder)
	{
		ffVideoEncoder->StopEncode();
		ffVideoEncoder.reset(nullptr);
	}

	if (audio_encoder_data)
	{
		delete[] audio_encoder_data;
		audio_encoder_data = nullptr;
	}
	if (video_encoder_data) {
		delete[] audio_encoder_data;
		audio_encoder_data = nullptr;
	}

	start_flag = false;

	if (rtmpPush)
	{
		rtmpPush->Close();
	}

}

void MediaPushWindow::setMic(const QAudioDeviceInfo& cameraInfo)
{
	m_mic.reset(new AudioCapture());

	connect(m_mic.data(), &AudioCapture::aframeAvailable, this, &MediaPushWindow::recvAFrame);
	
	//监听麦克风的level
	connect(m_mic.data(), &AudioCapture::updateLevel, [this]() {
			renderArea->setLevel(m_mic->level());
	});
	m_mic->Start(cameraInfo);
}

void MediaPushWindow::setCamera(const QCameraInfo& cameraInfo)
{
	m_camera.reset(new QCamera(cameraInfo));

	connect(m_camera.data(), &QCamera::stateChanged, this, &MediaPushWindow::updateCameraState);
	connect(m_camera.data(), QOverload<QCamera::Error>::of(&QCamera::error), this, &MediaPushWindow::displayCameraError);

	m_imageCapture.reset(new QCameraImageCapture(m_camera.data()));

	connect(ui.exposureCompensation, &QAbstractSlider::valueChanged, this, &MediaPushWindow::setExposureCompensation);

	if (showUserViewFinder)
	{
		m_camera->setViewfinder(ui.viewfinder);
	}
	else
	{
		//创建yuv采集器
		if (!m_pCameraInterface)
		{
			m_pCameraInterface.reset(new CBaseCameraSurface(this));
		}
		m_camera->setViewfinder(m_pCameraInterface.data());
		displayCapturedImage();
		connect(m_pCameraInterface.data(), SIGNAL(frameAvailable(QVideoFrame&)), this, SLOT(recvVFrame(QVideoFrame&)));
	}


	updateCameraState(m_camera->state());
	updateLockStatus(m_camera->lockStatus(), QCamera::UserRequest);

	connect(m_imageCapture.data(), &QCameraImageCapture::readyForCaptureChanged, this, &MediaPushWindow::readyForCapture);
	connect(m_imageCapture.data(), &QCameraImageCapture::imageCaptured, this, &MediaPushWindow::processCapturedImage);
	connect(m_imageCapture.data(), &QCameraImageCapture::imageSaved, this, &MediaPushWindow::imageSaved);
	connect(m_imageCapture.data(), QOverload<int, QCameraImageCapture::Error, const QString&>::of(&QCameraImageCapture::error),
		this, &MediaPushWindow::displayCaptureError);

	connect(m_camera.data(), QOverload<QCamera::LockStatus, QCamera::LockChangeReason>::of(&QCamera::lockStatusChanged),
		this, &MediaPushWindow::updateLockStatus);

	//QList<QVideoFrame::PixelFormat> list = m_camera->supportedViewfinderPixelFormats();
	
	ui.captureWidget->setTabEnabled(0, (m_camera->isCaptureModeSupported(QCamera::CaptureStillImage)));
	ui.captureWidget->setTabEnabled(1, (m_camera->isCaptureModeSupported(QCamera::CaptureVideo)));

	updateCaptureMode();
	m_camera->start();

}


void MediaPushWindow::recvAFrame(const char* data, qint64 len)
{
	if (aacEncoder)
	{
	   int ret_len = aacEncoder->Encode(data, len, audio_encoder_data);

	   if (rtmpPush&& ret_len > 0)
	   {
#ifdef RTMP_PUSH_USER_FFMPEG
		   rtmpPush->PushPacket(RtmpPush::MediaType::AUDIO,audio_encoder_data, ret_len);
#else
		   rtmpPush->SendAudio(audio_encoder_data, ret_len);
#endif
	   }
	}
}

void MediaPushWindow::recvVFrame(QVideoFrame& frame) {
	frame.map(QAbstractVideoBuffer::ReadOnly);

	// 获取 YUV 编码的原始数据
	QVideoFrame::PixelFormat pixelFormat = frame.pixelFormat();

	width_ = frame.width();
	height_ = frame.height();
	
	//需要写文件的时候才去转换数据
	if (ffVideoEncoder)
	{
		if (pixelFormat == QVideoFrame::Format_RGB32)
		{
		
			int width = frame.width();
			int height = frame.height();

			if (dst_yuv_420 == nullptr)
			{
				dst_yuv_420 = new uchar[width * height * 3 / 2];
				memset(dst_yuv_420, 128, width * height * 3 / 2);
			}


		
			int planeCount = frame.planeCount();
			int mapBytes = frame.mappedBytes();
			int rgb32BytesPerLine = frame.bytesPerLine();
			const uchar* data = frame.bits();
			if (data == NULL)
			{
				return;
			}

			QVideoFrame::FieldType fileType = frame.fieldType();

			int idx = 0;
			int idxu = 0;
			int idxv = 0;
			for (int i = height-1;i >=0 ;i--)
			{
				for (int j = 0;j < width;j++)
				{
					uchar b = data[(width*i+j) * 4];
					uchar g = data[(width * i + j) * 4 + 1];
					uchar r = data[(width * i + j) * 4 + 2];
					uchar a = data[(width * i + j) * 4 + 3];
				
					if (rgb_out_file) {
						//fwrite(&b, 1, 1, rgb_out_file);
						//fwrite(&g, 1, 1, rgb_out_file);
						//fwrite(&r, 1, 1, rgb_out_file);
						//fwrite(&a, 1, 1, rgb_out_file);
					}

					uchar y = RGB2Y(r, g, b);
					uchar u = RGB2U(r, g, b);
					uchar v = RGB2V(r, g, b);

					dst_yuv_420[idx++] = clip_value(y,0,255);
					if (j % 2 == 0 && i % 2 == 0) {
				
						dst_yuv_420[width*height + idxu++] = clip_value(u,0,255);
						dst_yuv_420[width*height*5/4 + idxv++] = clip_value(v,0,255);
					}
				}
			}

		
			int len = ffVideoEncoder->Encode(dst_yuv_420, video_encoder_data);

			if (rtmpPush&&len>0)
			{
#ifdef RTMP_PUSH_USER_FFMPEG
				rtmpPush->PushPacket(RtmpPush::MediaType::VIDEO,video_encoder_data, len);
#else
				rtmpPush->SendVideo(video_encoder_data, len);
#endif
			}
		}
		
#ifdef WRITE_CAPTURE_YUV
		if (yuv_out_file) {
			fwrite(dst_yuv_420, 1, width * height * 3 / 2, yuv_out_file);
		}
#endif // WRITE_CAPTURE_YUV

	}
	
	if (!showUserViewFinder)
	{
		QImage::Format qImageFormat = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
		QImage videoImg(frame.bits(),
			frame.width(),
			frame.height(),
			qImageFormat);

		videoImg = videoImg.convertToFormat(qImageFormat).mirrored(false, true);


		ui.lastImagePreviewLabel->setPixmap(QPixmap::fromImage(videoImg));
	}

}


void MediaPushWindow::updateRecorderState(QMediaRecorder::State state)
{
	switch (state) {
	case QMediaRecorder::StoppedState:
		ui.recordButton->setEnabled(true);
		ui.pauseButton->setEnabled(true);
		ui.stopButton->setEnabled(false);
		break;
	case QMediaRecorder::PausedState:
		ui.recordButton->setEnabled(true);
		ui.pauseButton->setEnabled(false);
		ui.stopButton->setEnabled(true);
		break;
	case QMediaRecorder::RecordingState:
		ui.recordButton->setEnabled(false);
		ui.pauseButton->setEnabled(true);
		ui.stopButton->setEnabled(true);
		break;
	}
}

void MediaPushWindow::configureCaptureSettings()
{
	switch (m_camera->captureMode()) {
	case QCamera::CaptureStillImage:
		configureImageSettings();
		break;
	case QCamera::CaptureVideo:
		//configureVideoSettings(); 暂时不支持视频流捕获
		break;
	default:
		break;
	}
}

void MediaPushWindow::configureImageSettings()
{
	ImageSettings settingsDialog(m_imageCapture.data());
	settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

	settingsDialog.setImageSettings(m_imageSettings);

	if (settingsDialog.exec()) {
		m_imageSettings = settingsDialog.imageSettings();
		m_imageCapture->setEncodingSettings(m_imageSettings);
	}
}

void MediaPushWindow::displayViewfinder()
{
	ui.stackedWidget->setCurrentIndex(0);
}


void MediaPushWindow::takeImage()
{
	m_isCapturingImage = true;
	m_imageCapture->capture();
}

void MediaPushWindow::toggleLock()
{
	switch (m_camera->lockStatus()) {
	case QCamera::Searching:
	case QCamera::Locked:
		m_camera->unlock();
		break;
	case QCamera::Unlocked:
		m_camera->searchAndLock();
	}
}

void MediaPushWindow::imageSaved(int id, const QString& fileName)
{
	Q_UNUSED(id);
	ui.statusbar->showMessage(tr("Captured \"%1\"").arg(QDir::toNativeSeparators(fileName)));

	m_isCapturingImage = false;
	if (m_applicationExiting)
		close();
}



void MediaPushWindow::closeEvent(QCloseEvent* event)
{
	if (m_isCapturingImage) {
		setEnabled(false);
		m_applicationExiting = true;
		event->ignore();
	}
	else {
		event->accept();
	}
}

void MediaPushWindow::displayCaptureError(int id, const QCameraImageCapture::Error error, const QString& errorString)
{
	Q_UNUSED(id);
	Q_UNUSED(error);
	QMessageBox::warning(this, tr("Image Capture Error"), errorString);
	m_isCapturingImage = false;
}

void MediaPushWindow::processCapturedImage(int requestId, const QImage& img)
{

	Q_UNUSED(requestId);
	QImage scaledImage = img.scaled(ui.viewfinder->size(),
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation);

	ui.lastImagePreviewLabel->setPixmap(QPixmap::fromImage(scaledImage));

	// Display captured image for 4 seconds.
	displayCapturedImage();

	if (showUserViewFinder) {
       QTimer::singleShot(4000, this, &MediaPushWindow::displayViewfinder);
	}
	
}

void MediaPushWindow::displayCapturedImage()
{
	ui.stackedWidget->setCurrentIndex(1);
}

void MediaPushWindow::readyForCapture(bool ready)
{
	ui.takeImageButton->setEnabled(ready);
}

void MediaPushWindow::updateLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
	QColor indicationColor = Qt::black;

	switch (status) {
	case QCamera::Searching:
		indicationColor = Qt::yellow;
		ui.statusbar->showMessage(tr("Focusing..."));
		ui.lockButton->setText(tr("Focusing..."));
		break;
	case QCamera::Locked:
		indicationColor = Qt::darkGreen;
		ui.lockButton->setText(tr("Unlock"));
		ui.statusbar->showMessage(tr("Focused"), 2000);
		break;
	case QCamera::Unlocked:
		indicationColor = reason == QCamera::LockFailed ? Qt::red : Qt::black;
		ui.lockButton->setText(tr("Focus"));
		if (reason == QCamera::LockFailed)
			ui.statusbar->showMessage(tr("Focus Failed"), 2000);
	}

	QPalette palette = ui.lockButton->palette();
	palette.setColor(QPalette::ButtonText, indicationColor);
	ui.lockButton->setPalette(palette);
}

void MediaPushWindow::setExposureCompensation(int index)
{
	m_camera->exposure()->setExposureCompensation(index * 0.5);
}

void MediaPushWindow::displayCameraError()
{
	QMessageBox::warning(this, tr("Camera Error"), m_camera->errorString());
}

void MediaPushWindow::updateCameraState(QCamera::State state)
{
	switch (state) {
	case QCamera::ActiveState:
		/*ui.actionStart->setEnabled(false);
		ui.actionStop->setEnabled(true);*/
		ui.captureWidget->setEnabled(true);
		ui.actionSettings->setEnabled(true);
		break;
	case QCamera::UnloadedState:
	case QCamera::LoadedState:
		//ui.actionStart->setEnabled(true);
		//ui.actionStop->setEnabled(false);
		ui.captureWidget->setEnabled(false);
		ui.actionSettings->setEnabled(false);
	}
}

void MediaPushWindow::updateCameraDevice(QAction* action)
{
	setCamera(qvariant_cast<QCameraInfo>(action->data()));
}
void MediaPushWindow::updateMicDevice(QAction* action)
{
	setMic(qvariant_cast<QAudioDeviceInfo>(action->data()));
}

void MediaPushWindow::updateCaptureMode()
{
	int tabIndex = ui.captureWidget->currentIndex();
	QCamera::CaptureModes captureMode = tabIndex == 0 ? QCamera::CaptureStillImage : QCamera::CaptureVideo;

	if (m_camera->isCaptureModeSupported(captureMode))
		m_camera->setCaptureMode(captureMode);
}


MediaPushWindow::~MediaPushWindow()
{}
