#pragma once

#include <QtWidgets/QMainWindow>
#include <QCameraInfo>
#include <QCamera>
#include "uic/ui_MediaPushWindow.h"
#include <QScopedPointer>
#include <QCameraImageCapture>
#include <QMediaRecorder>
#include <QKeyEvent>
#include <QVideoProbe>
#include "imagesettings.h"
#include "CBaseCameraSurface.h"
#include "AudioCapture.h"


#define USE_FFMPEG_VIDEO_ENCODE 1;

#if USE_FFMPEG_VIDEO_ENCODE
#include "VideoEncodeFF.h"
#else
#include "VideoEncoderX.h"
#endif
#include "AacEncoder.h"

//#define WRITE_CAPTURE_YUV

#define RGB2Y(r,g,b) \
	((unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8)+16)

#define RGB2U(r,g,b) \
	((unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128)

#define RGB2V(r,g,b) \
	((unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128)


class MediaPushWindow : public QMainWindow
{
	Q_OBJECT

public:
	MediaPushWindow(QWidget* parent = nullptr);
	~MediaPushWindow();
private slots:
	void setCamera(const QCameraInfo& cameraInfo);
	void setMic(const QAudioDeviceInfo& cameraInfo);

	void start();
	void stop();

	void toggleLock();
	void takeImage();
	void displayCaptureError(int, QCameraImageCapture::Error, const QString& errorString);

	void configureCaptureSettings();

	void configureImageSettings();

	void displayCameraError();

	void updateCameraDevice(QAction* action);
	void updateMicDevice(QAction* action);

	void updateCameraState(QCamera::State);
	void updateCaptureMode();
	void updateRecorderState(QMediaRecorder::State state);
	void setExposureCompensation(int index);

	void processCapturedImage(int requestId, const QImage& img);
	void updateLockStatus(QCamera::LockStatus, QCamera::LockChangeReason);

	void displayViewfinder();
	void displayCapturedImage();

	void readyForCapture(bool ready);
	void imageSaved(int id, const QString& fileName);

	void recvVFrame(QVideoFrame& frame);
	void recvAFrame(const char* data, qint64 len);
protected:

	void closeEvent(QCloseEvent* event) override;
private:
	void InitAudioEncode();

private:
	Ui::Camera ui;

	//因YUV的范围是0~255因此在其范围外的数值都做截断处理
	inline unsigned char clip_value(unsigned char x, unsigned char min_val, unsigned char  max_val) {
		if (x > max_val) {
			return max_val; //大于max_val的值记作max_val
		}
		else if (x < min_val) {
			return min_val;  //小于min_val的值记作min_val
		}
		else {
			return x; //在指定范围内，就保持不变
		}
	}

	QScopedPointer<AudioCapture> m_mic;
	QScopedPointer<QCamera> m_camera;
	QScopedPointer<QCameraImageCapture> m_imageCapture;
	//QScopedPointer<QMediaRecorder> m_mediaRecorder;

	QImageEncoderSettings m_imageSettings;

	bool m_isCapturingImage = false;
	bool m_applicationExiting = false;

	bool start_flag = false;
#ifdef WRITE_CAPTURE_YUV
	FILE* yuv_out_file = nullptr;
#endif // WRITE_CAPTURE_YUV


	FILE* rgb_out_file = nullptr;


	unsigned char* dst_yuv_420 = nullptr;

	bool showUserViewFinder = false;
	QScopedPointer<CBaseCameraSurface> m_pCameraInterface;

	RenderArea* renderArea;

#if USE_FFMPEG_VIDEO_ENCODE
	QScopedPointer<VideoEncodeFF> ffVideoEncoder;
#else
	QScopedPointer<VideoEncoderX> ffVideoEncoder;
#endif

	QScopedPointer<AacEncoder> aacEncoder;

	
};
