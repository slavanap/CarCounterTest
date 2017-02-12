#pragma once

#include <QBasicTimer>
#include <QDebug>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QTimerEvent>
#include <vector>
#include <opencv2/opencv.hpp>

#include "QtUtility.h"

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
	CarDescriptor();
	CarDescriptor(const std::vector<cv::Point>& contour);
	void predictNextPosition(void);
	bool isCar() const;
	void assign(const CarDescriptor& other);
};

class DetectFilter : public AbstractFilter {
	Q_OBJECT
	Q_PROPERTY(QVector<QLineF> segments READ segments WRITE setSegments)

public:
	explicit DetectFilter(QObject* parent = nullptr);

	QVector<QLineF> segments() const {
		QMutexLocker lock(&_mutex);
		return _segments;
	}
	Q_SLOT void setSegments(const QVector<QLineF>& segments) {
		QMutexLocker lock(&_mutex);
		_segments = segments;
	}

protected:
	 bool process(cv::Mat& mat) override;

private:
	mutable QMutex _mutex;
	QVector<QLineF> _segments;

	std::vector<CarDescriptor> _cars, _currentFrameCars;
	cv::Mat _prevFrame;
	cv::Mat _structuringElement;
	int _carCount;
};
