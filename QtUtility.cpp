#include <QTimerEvent>

#include "QtUtility.h"

// class QtCVImage

const QImage& QtCVImage::image() const {
	if (_image.isNull() && !_mat.empty()) {
		QImage::Format format;
		switch (_mat.type()) {
			case CV_8UC1: format = QImage::Format_Indexed8; break;
			case CV_8UC3: format = QImage::Format_RGB888; break;
			default:
				_image = QImage();
				return _image;
		}
		_image = QImage(_mat.data, _mat.cols, _mat.rows, (int)_mat.step, format,
			[](void *ptr) { delete static_cast<cv::Mat*>(ptr); },
			new cv::Mat(_mat)
		);
		if (_mat.type() == CV_8UC1) {
			QVector<QRgb> colorTable;
			colorTable.reserve(256);
			for (int i = 0; i < 256; i++)
				colorTable.push_back(qRgb(i, i, i));
			_image.setColorTable(colorTable);
		}
	}
	return _image;
}

const cv::Mat& QtCVImage::mat() const {
	if (_mat.empty() && !_image.isNull()) {
		_mat = cv::Mat(_image.height(), _image.width(), CV_8UC3, (void*)_image.constScanLine(0), _image.bytesPerLine());
	}
	return _mat;
}

QtCVImage& QtCVImage::operator=(const QtCVImage& other) {
	_image = other._image;
	_mat = other._mat;
	return *this;
}

QtCVImage& QtCVImage::operator=(const QImage& image) {
	_image = image.convertToFormat(QImage::Format_RGB888);
	_mat.release();
	return *this;
}

QtCVImage& QtCVImage::operator=(const cv::Mat& mat) {
	_image = QImage();
	if (_mat.type() == CV_8UC3)
		cv::cvtColor(mat, _mat, CV_BGR2RGB);
	else
	_mat = mat;
	return *this;
}

QtCVImage& QtCVImage::operator=(const QVideoFrame& frame) {
	_frame = frame;
	if (_frame.map(QAbstractVideoBuffer::ReadOnly)) {
		QImage::Format format = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
		_image = QImage(frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), format);
	}
	return *this;
}



// class AbstractFilter

void AbstractFilter::start() {
	if (_surface != nullptr && _videoCapture != nullptr) {
		_frameCount = 0;
		_surface->start(_format);
	}
	_timer.start((int)(1000 / _format.frameRate()), this);
}

void AbstractFilter::stop() {
	_timer.stop();
	if (_surface != nullptr && _surface->isActive())
		_surface->stop();
}


void AbstractFilter::setVideoSurface(QAbstractVideoSurface *surface) {
	if (_surface == surface)
		return;
	stop();
	_surface = surface;
	if (!_timer.isActive())
		start();
}

void AbstractFilter::timerEvent(QTimerEvent* ev) {
	if (ev->timerId() != _timer.timerId())
		return;
	cv::Mat frame;
	// Next statement blocks until a new frame is ready
	if (!_videoCapture->read(frame)) {
		_timer.stop();
		return;
	}
	_frameCount++;
	if (process(frame)) {
		QtCVImage i(frame);
		if (_surface != nullptr)
			_surface->present(QVideoFrame(i.image()));
		else
			emit newFrame(i.image());
	}
}

bool AbstractFilter::open(cv::VideoCapture* capturePtr) {
	if (_timer.isActive())
		stop();
	if (!capturePtr->isOpened())
		return false;
	_videoCapture.reset(capturePtr);

	// initialize _format
	_format = QVideoSurfaceFormat(
		QSize((int)capturePtr->get(cv::CAP_PROP_FRAME_WIDTH), (int)capturePtr->get(cv::CAP_PROP_FRAME_HEIGHT)),
		QVideoFrame::Format_RGB24);
	// OpenCV returns 1000 = capturePtr->get(cv::CAP_PROP_FPS)
	_format.setFrameRate(25);
	start();
	return true;
}

bool AbstractFilter::open(const QString& filename) {
	return open(new cv::VideoCapture(filename.toStdString()));
}

bool AbstractFilter::open(int cvCamId) {
	return open(new cv::VideoCapture(cvCamId));
}
