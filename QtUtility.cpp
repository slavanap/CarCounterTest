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

#ifdef VIDEOFRAME_SUPPORT
QtCVImage& QtCVImage::operator=(const QVideoFrame& frame) {
	_frame = frame;
	if (_frame.map(QAbstractVideoBuffer::ReadOnly)) {
		QImage::Format format = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
		_image = QImage(frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), format);
	}
	return *this;
}
#endif



// class AbstractFilter

void AbstractFilter::start() {
	if (!_timer.isActive()) {
		_frameCount = 0;
		_timer.start((int)(1001 / 24), this);
	}
}

void AbstractFilter::stop() {
	_timer.stop();
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
		emit newFrame(i.image());
	}
}

bool AbstractFilter::open(cv::VideoCapture* capturePtr) {
	if (_timer.isActive())
		stop();
	if (!capturePtr->isOpened())
		return false;
	_videoCapture.reset(capturePtr);

	start();
	return true;
}

bool AbstractFilter::open(const QString& filename) {
	return open(new cv::VideoCapture(filename.toStdString()));
}

bool AbstractFilter::open(int cvCamId) {
	return open(new cv::VideoCapture(cvCamId));
}





// Given three colinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
inline bool onSegment(const QPointF& p, const QPointF& q, const QPointF& r) {
	return
		q.x() <= std::max(p.x(), r.x()) && q.x() >= std::min(p.x(), r.x()) &&
		q.y() <= std::max(p.y(), r.y()) && q.y() >= std::min(p.y(), r.y());
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int orientation(const QPointF& p, const QPointF& q, const QPointF& r)
{
	// See http://www.geeksforgeeks.org/orientation-3-ordered-points/
	// for details of below formula.
	int val = (q.y() - p.y()) * (r.x() - q.x()) - (q.x() - p.x()) * (r.y() - q.y());
	if (val == 0)
		return 0;  // colinear
	return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool intersects(const QLineF& l1, const QLineF& l2, bool& directionDown) {
	const QPointF &p1 = l1.p1(), &q1 = l1.p2(), &p2 = l2.p1(), &q2 = l2.p2();

	// Find the four orientations needed for general and special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	directionDown = o3 == 2;

	// General case
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases
	// p1, q1 and p2 are colinear and p2 lies on segment p1q1
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and p2 are colinear and q2 lies on segment p1q1
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	 // p2, q2 and q1 are colinear and q1 lies on segment p2q2
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases
}
