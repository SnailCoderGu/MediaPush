#include "CBaseCameraSurface.h"
#include <QDebug>

CBaseCameraSurface::CBaseCameraSurface(QObject* parent) : QAbstractVideoSurface(parent)
{

}

CBaseCameraSurface::~CBaseCameraSurface() {}

bool CBaseCameraSurface::isFormatSupported(const QVideoSurfaceFormat& format) const {
	return QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid;
}

QList<QVideoFrame::PixelFormat> CBaseCameraSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
	if (handleType == QAbstractVideoBuffer::NoHandle) {
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32
			<< QVideoFrame::Format_ARGB32
			<< QVideoFrame::Format_ARGB32_Premultiplied
			<< QVideoFrame::Format_RGB565
			<< QVideoFrame::Format_RGB555;
		//这里添加更多的类型，因为有很多摄像头返回的数据类型是yuv格式的
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool CBaseCameraSurface::start(const QVideoSurfaceFormat& format) {
	//要通过QCamera::start()启动
	if (QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid
		&& !format.frameSize().isEmpty()) {
		QAbstractVideoSurface::start(format);
		return true;
	}
	return false;
}

void CBaseCameraSurface::stop() {
	QAbstractVideoSurface::stop();
}

bool CBaseCameraSurface::present(const QVideoFrame& frame) {
	//QCamera::start()启动之后才会触发
	//qDebug()<<"camera present";
	if (!frame.isValid()) {
		qDebug() << "get invalid frame";
		stop();
		return false;
	}

	QVideoFrame cloneFrame(frame);
	emit frameAvailable(cloneFrame);
	return true;
}


