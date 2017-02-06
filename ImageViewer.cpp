#include "ImageViewer.h"

#include <QDebug>
#include <QPainter>

void ImageViewer::paintEvent(QPaintEvent* ev) {
	Q_UNUSED(ev)
	QMutexLocker locker(&_mutex);
	QImage image = std::move(_image);
	locker.unlock();

	QPainter p(this);
	p.drawImage(0, 0, image);
}

ImageViewer::ImageViewer(QWidget* parent) :
	QWidget(parent),
	_mutex(QMutex::Recursive)
{
	setAttribute(Qt::WA_OpaquePaintEvent);
}

void ImageViewer::setImage(const QImage& image) {
	{
		QMutexLocker locker(&_mutex);
		if (!_image.isNull())
			qDebug() << "Viewer dropped frame!";
		_image = image;
		if (_image.size() != size())
			setFixedSize(_image.size());
	}
	update();
}
