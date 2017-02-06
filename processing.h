#pragma once

#include <QBasicTimer>
#include <QDebug>
#include <QImage>
#include <QObject>
#include <QTimerEvent>

#include <vector>

#include <opencv2/opencv.hpp>


Q_DECLARE_METATYPE(cv::Mat)

struct CarDescriptor {
	std::vector<cv::Point> contour;
	cv::Rect boundingRect;
	std::vector<cv::Point> centerPositions;
	double diagonalSize;
	double aspectRatio;
	bool isMatchFound;
	bool isTracked;
	int numFramesWithoutMatch;
	cv::Point predictedNextPos;
	CarDescriptor(const std::vector<cv::Point>& contour);
	void predictNextPosition(void);
	bool isCar() const;
	void assign(const CarDescriptor& other);
};

class DetectFilter : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dropFrames READ dropFrames WRITE setDropFrames)
public:
	explicit DetectFilter(QObject* parent = nullptr);
	bool dropFrames() const { return _dropFrames; }
	void setDropFrames(bool value) { _dropFrames = value; }

	Q_SLOT void startCam(int cam) {
		_videoCapture.reset(new cv::VideoCapture(cam));
		start();
	}
	Q_SLOT void startFile(const QString& filename) {
		_videoCapture.reset(new cv::VideoCapture(filename.toStdString()));
		start();
	}
	Q_SLOT void stop() {
		_timer.stop();
	}
	Q_SLOT void setSegments(const QVector<QLineF>& segments);

	/*
	Q_SLOT void processFrame(const cv::Mat& frame) {
		if (_dropFrames)
			queue(frame);
		else
			process(frame);
	}
	*/

	Q_SIGNAL void imageReady(QImage image);

private:
	QBasicTimer _timer;
	QScopedPointer<cv::VideoCapture> _videoCapture;
	void timerEvent(QTimerEvent* ev) override {
		if (ev->timerId() != _timer.timerId())
			return;
		cv::Mat frame;
		if (!_videoCapture->read(frame)) { // Blocks until a new frame is ready
			_timer.stop();
			return;
		}
		process(frame);
	}
	void start() {
		if (_videoCapture->isOpened()) {
			_timer.start(0, this);
		}
	}

	cv::Mat _cvFrame;
	bool _dropFrames;

	std::vector<CarDescriptor> _cars, _currentFrameCars;
	cv::Mat _prevFrame;
	cv::Mat _structuringElement;
	int _frameCount;
	int _carCount;

	static void matDeleter(void* mat) {
		delete static_cast<cv::Mat*>(mat);
	}

	void process(cv::Mat frame);

	/*
	void queue(const cv::Mat& frame) {
#ifndef NDEBUG
		if (!_cvFrame.empty())
			qDebug() << "Framedrop on processing";
#endif
		_cvFrame = frame;
		if (!_timer.isActive())
			_timer.start(0, this);
	}

	void timerEvent(QTimerEvent* ev) override {
		if (ev->timerId() != _timer.timerId())
			return;
		process(_cvFrame);
		_cvFrame.release();
		_timer.stop();
	}
	*/
};
