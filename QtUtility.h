#pragma once

#include <QBasicTimer>
#include <QImage>
#ifdef VIDEOFRAME_SUPPORT
#	include <QVideoFrame>
#endif
#include <opencv2/opencv.hpp>

class QtCVImage {
public:
	QtCVImage(const QImage& image) { operator=(image); }
	QtCVImage(const cv::Mat& mat) { operator=(mat); }

	const QImage& image() const;
	const cv::Mat& mat() const;
	QtCVImage& operator=(const QtCVImage& other);
	QtCVImage& operator=(const QImage& image);
	QtCVImage& operator=(const cv::Mat& mat);
	operator QImage() const { return image(); }
	operator cv::Mat() const { return mat(); }

private:
	mutable QImage _image;
	mutable cv::Mat _mat;

#ifdef VIDEOFRAME_SUPPORT
public:
	QtCVImage(const QVideoFrame& frame) { operator=(frame); }
	QtCVImage& operator=(const QVideoFrame& frame);
private:
	QVideoFrame _frame;
#endif

};

class AbstractFilter : public QObject {
	Q_OBJECT
public:
	explicit AbstractFilter(QObject* parent = nullptr) :
		QObject(parent),
		_frameCount(0)
	{
		// empty
	}
	Q_SLOT bool open(const QString& filename);
	Q_SLOT bool open(int cvCamId);
	Q_SIGNAL void newFrame(const QImage& image);

protected:
	int _frameCount;

	virtual bool process(cv::Mat& mat) {
		Q_UNUSED(mat);
		return true; // use false to skip the frame
	}

private:
	QScopedPointer<cv::VideoCapture> _videoCapture;
	QBasicTimer _timer;
	void timerEvent(QTimerEvent* ev) override;
	bool open(cv::VideoCapture* capturePtr);
	void start();
	void stop();

};

bool intersects(const QLineF& l1, const QLineF& l2, bool& directionDown);
