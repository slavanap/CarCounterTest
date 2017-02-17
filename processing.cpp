#define SHOW_STEPS 1

#include <algorithm>
#include <iterator>
#include <vector>
#include "processing.h"

CarDescriptor::CarDescriptor() :
	isMatchFound(true),
	isCounted(false),
	numFramesWithoutMatch(0)
{
	// empty
}

CarDescriptor::CarDescriptor(const std::vector<cv::Point>& contour) :
	contour(contour),
	boundingRect(cv::boundingRect(contour)),
	isMatchFound(true),
	isCounted(false),
	numFramesWithoutMatch(0)
{
	cv::Point center(
		boundingRect.x + boundingRect.width / 2,
		boundingRect.y + boundingRect.height / 2);
	centerPositions.push_back(center);
	diagonalSize = sqrt(pow(boundingRect.width, 2) + pow(boundingRect.height, 2));
	aspectRatio = (double)boundingRect.width / boundingRect.height;
}

void CarDescriptor::predictNextPosition() {
	int account = std::min(5, (int)centerPositions.size());
	auto prev = centerPositions.rbegin();
	auto current = prev;
	std::advance(current, 1);
	int deltaX = 0, deltaY = 0, sum = 0;
	for (int i = 1; i < account; ++i) {
		deltaX += (current->x - prev->x) * i;
		deltaY += (current->y - prev->y) * i;
		sum += i;
	}
	if (sum > 0) {
		deltaX /= sum;
		deltaY /= sum;
	}
	predictedNextPos.x = centerPositions.back().x + deltaX;
	predictedNextPos.y = centerPositions.back().y + deltaY;
}

bool CarDescriptor::isCar() const {
	return boundingRect.area() > 600 &&
		3.0 < aspectRatio && aspectRatio < 20.0 &&
		boundingRect.width > 40 &&
		diagonalSize > 70.0 &&
		cv::contourArea(contour)/boundingRect.area() > 0.5;
}

void CarDescriptor::assign(const CarDescriptor& other) {
	contour = other.contour;
	boundingRect = other.boundingRect;
	centerPositions.push_back(other.centerPositions.back());
	diagonalSize = other.diagonalSize;
	aspectRatio = other.aspectRatio;
	isMatchFound = true;
}


const cv::Scalar BLACK = cv::Scalar(0.0, 0.0, 0.0);
const cv::Scalar WHITE = cv::Scalar(255.0, 255.0, 255.0);
const cv::Scalar YELLOW = cv::Scalar(0.0, 255.0, 255.0);
const cv::Scalar GREEN = cv::Scalar(0.0, 200.0, 0.0);
const cv::Scalar RED = cv::Scalar(0.0, 0.0, 255.0);

double distance(const cv::Point& p1, const cv::Point& p2) {
	int intX = p1.x - p2.x;
	int intY = p1.y - p2.y;
	return sqrt((double)(intX * intX + intY * intY));
}

void matchCars(std::list<CarDescriptor>& existing, const std::list<CarDescriptor>& current) {
	for (auto &ex : existing) {
		ex.isMatchFound = false;
		ex.predictNextPosition();
	}
	for (auto &cur : current) {
		std::list<CarDescriptor>::iterator itOfLeastDistance = existing.begin();
		double leastDistance = std::numeric_limits<double>::max();
		for (auto it = existing.begin(); it != existing.end(); ++it) {
			double v = distance(cur.centerPositions.back(), it->predictedNextPos);
			if (v < leastDistance) {
				leastDistance = v;
				itOfLeastDistance = it;
			}
		}
		if (leastDistance < cur.diagonalSize * 0.5) {
			itOfLeastDistance->assign(cur);
		}
		else {
			existing.emplace_back(cur);
			existing.back().isMatchFound = true;
		}
	}
	for (auto it = existing.begin(); it != existing.end(); ) {
		if (!it->isMatchFound) {
			if (++it->numFramesWithoutMatch >= 5) {
				it = existing.erase(it);
				continue;
			}
		}
		++it;
	}
}

#ifdef SHOW_STEPS

void show(const cv::Size& imageSize, const std::vector<std::vector<cv::Point>>& contours, const std::string& title) {
	cv::Mat image(imageSize, CV_8UC3, BLACK);
	cv::drawContours(image, contours, -1, WHITE, -1);
	cv::imshow(title, image);
}

void show(const cv::Size& imageSize, const std::list<CarDescriptor>& cars, const std::string& title) {
	cv::Mat image(imageSize, CV_8UC3, BLACK);
	std::vector<std::vector<cv::Point>> contours;
	for (auto &car : cars)
		contours.push_back(car.contour);
	cv::drawContours(image, contours, -1, WHITE, -1);
	cv::imshow(title, image);
}

#endif

inline void drawCarsInfo(const std::list<CarDescriptor>& cars, cv::Mat& image) {
	for (auto car = cars.begin(); car != cars.end(); ++car)
		cv::rectangle(image, car->boundingRect, RED, 2);
}

DetectFilter::DetectFilter(QObject* parent) :
	AbstractFilter(parent),
	_mutex(QMutex::Recursive),
	_carsCount(0)
{
	_structuringElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
}

bool DetectFilter::process(cv::Mat& currentFrame) {
	_frameCount++;
	cv::resize(currentFrame, currentFrame, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
	if (_prevFrame.empty()) {
		_prevFrame = currentFrame;
		return true;
	}
	_currentFrameCars.resize(0);

	cv::Mat prevFrameCopy, curFrameCopy, imgDifference, imgThresh;
	cv::cvtColor(currentFrame, curFrameCopy, CV_BGR2GRAY);
	//cv::GaussianBlur(curFrameCopy, curFrameCopy, cv::Size(5,5), 0);
//	cv::cvtColor(_prevFrame, prevFrameCopy, CV_BGR2GRAY);
//	cv::GaussianBlur(prevFrameCopy, prevFrameCopy, cv::Size(5,5), 0);
//	cv::absdiff(prevFrameCopy, curFrameCopy, imgDifference);
	cv::threshold(curFrameCopy, imgThresh, 100, 255.0, CV_THRESH_BINARY);
//	for (int i = 0; i < 3; i++) {
		cv::dilate(imgThresh, imgThresh, _structuringElement);
//		cv::dilate(imgThresh, imgThresh, _structuringElement);
//		cv::erode(imgThresh, imgThresh, _structuringElement);
//	}
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(imgThresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_KCOS);
#if SHOW_STEPS
	show(imgThresh.size(), contours, "contours");
#endif

	std::vector<std::vector<cv::Point>> convexHulls(contours.size());
	for (int i = 0; i < contours.size(); i++)
		cv::convexHull(contours[i], convexHulls[i]);
#if SHOW_STEPS
	show(imgThresh.size(), convexHulls, "convexHulls");
#endif

	for (auto &convexHull : convexHulls) {
		CarDescriptor car(convexHull);
		if (car.isCar())
			_currentFrameCars.push_back(car);
	}
#if SHOW_STEPS
	show(imgThresh.size(), _currentFrameCars, "currentCars");
#endif

	if (_frameCount <= 2)
		_cars.swap(_currentFrameCars);
	else {
		matchCars(_cars, _currentFrameCars);
	}
#if SHOW_STEPS
	show(imgThresh.size(), _cars, "trackedCars");
#endif

	_prevFrame = currentFrame;
	currentFrame = currentFrame.clone();

	// prepare visualization
	drawCarsInfo(_cars, currentFrame);

	double fontScale = (currentFrame.rows * currentFrame.cols) / 1000000.0;
	int fontThickness = (int)std::round(fontScale * 1.5);

	// for each line
	QMutexLocker lock(&_mutex);
	for (int i = 0; i < _segments.size(); ++i) {
		const QLineF &line = _segments[i];
		bool highlight = false;
		for (CarDescriptor &car : _cars) {
			if (!car.isCounted && car.centerPositions.size() >= 2) {
				bool directionDown;
				const cv::Point
					&p2 = car.centerPositions[(int)car.centerPositions.size() - 2],
					&q2 = car.centerPositions[(int)car.centerPositions.size() - 1];
				if (intersects(line, QLineF(p2.x, p2.y, q2.x, q2.y), directionDown)) {
					if (directionDown) {
						++_carsCount[i];
						highlight = true;
						car.isCounted = true;
					}
				}
			}
		}
		QPointF center = line.center();
		cv::line(currentFrame, cv::Point((int)line.x1(), (int)line.y1()), cv::Point((int)line.x2(), (int)line.y2()), highlight ? GREEN : RED, 2);
		cv::putText(currentFrame, std::to_string(_carsCount[i]), cv::Point((int)center.x(), (int)center.y()), CV_FONT_HERSHEY_SIMPLEX, fontScale, YELLOW, fontThickness);
	}
	//cv::resize(result, result, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
	return true;
}
