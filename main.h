#pragma once
#include <QAbstractVideoFilter>

class CarFilterRunnable;

class CarFilter : public QAbstractVideoFilter {
	Q_OBJECT
	Q_PROPERTY(QVector<QLine> *segments READ segments WRITE setSegments)

public:
	CarFilter() { }
	QVideoFilterRunnable *createFilterRunnable() override;
	QVector<QLine> segments() const { return _segments; }
	void setSegments(const QVector<QLine>& values) { _segments = segments; }

private:
	friend class CarFilterRunnable;
	QVector<QLine> _segments;

};

class CarFilterRunnable : public QVideoFilterRunnable {
public:
	CarFilterRunnable(CarFilter *filter);
	~CarFilterRunnable();
	QVideoFrame run(QVideoFrame* input, const QVideoSurfaceFormat& surfaceFormat, RunFlags flags) override;

private:

};

inline QVideoFilterRunnable *CarFilter::createFilterRunnable() {
	return new CarFilterRunnable(this);
}

