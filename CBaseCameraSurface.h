#ifndef CBASECAMERA_H
#define CBASECAMERA_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class CBaseCameraSurface : public QAbstractVideoSurface
{
	Q_OBJECT
public:
	explicit CBaseCameraSurface(QObject* parent = nullptr);
	~CBaseCameraSurface();

	virtual bool isFormatSupported(const QVideoSurfaceFormat& format) const override;
	virtual bool present(const QVideoFrame& frame) override;
	virtual bool start(const QVideoSurfaceFormat& format) override;
	virtual void stop() override;
	virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override;

signals:
	void frameAvailable(QVideoFrame& frame);

public slots:
};

#endif // CBASECAMERA_H