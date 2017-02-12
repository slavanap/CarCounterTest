#pragma once

#include <QAbstractVideoSurface>
#include <QBasicTimer>
#include <QImage>
#include <QVideoFrame>
#include <QVideoSurfaceFormat>
#include <opencv2/opencv.hpp>

class QtCVImage {
public:
	QtCVImage(const QImage& image) { operator=(image); }
	QtCVImage(const cv::Mat& mat) { operator=(mat); }
	QtCVImage(const QVideoFrame& frame) { operator=(frame); }
	
	const QImage& image() const;
	const cv::Mat& mat() const;
	QtCVImage& operator=(const QtCVImage& other);
	QtCVImage& operator=(const QImage& image);
	QtCVImage& operator=(const cv::Mat& mat);
	QtCVImage& operator=(const QVideoFrame& frame);
	operator QImage() const { return image(); }
	operator cv::Mat() const { return mat(); }
private:
	QVideoFrame _frame;
	mutable QImage _image;
	mutable cv::Mat _mat;
};

class AbstractFilter : public QObject {
	Q_OBJECT
	Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)
public:
	explicit AbstractFilter(QObject* parent = nullptr) :
		QObject(parent),
		_frameCount(0),
		_surface(nullptr)
	{
		// empty
	}
	Q_SLOT bool open(const QString& filename);
	Q_SLOT bool open(int cvCamId);
	QAbstractVideoSurface* videoSurface() const { return _surface; }
	void setVideoSurface(QAbstractVideoSurface* surface);
	Q_SIGNAL void newFrame(const QImage& image);

protected:
	int _frameCount;

	virtual bool process(cv::Mat& mat) {
		Q_UNUSED(mat);
		return true; // use false to skip the frame
	}

private:
	QAbstractVideoSurface *_surface;
	QVideoSurfaceFormat _format;
	QScopedPointer<cv::VideoCapture> _videoCapture;
	QBasicTimer _timer;
	void timerEvent(QTimerEvent* ev) override;
	bool open(cv::VideoCapture* capturePtr);
	void start();
	void stop();

};
