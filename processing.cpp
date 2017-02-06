#define SHOW_STEPS 0

#include <algorithm>
#include <iterator>
#include <vector>
#include "processing.h"

CarDescriptor::CarDescriptor(const std::vector<cv::Point>& contour) :
	contour(contour),
	boundingRect(cv::boundingRect(contour)),
	isMatchFound(true),
	isTracked(true),
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
		0.2 < aspectRatio && aspectRatio < 4.0 &&
		boundingRect.width > 40 && boundingRect.height > 40 &&
		diagonalSize > 70.0 &&
		cv::contourArea(contour)/boundingRect.area() > 0.5;
}

void CarDescriptor::assign(const CarDescriptor& other) {
	contour = other.contour;
	boundingRect = other.boundingRect;
	centerPositions.push_back(other.centerPositions.back());
	diagonalSize = other.diagonalSize;
	aspectRatio = other.aspectRatio;
	isTracked = true;
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

void matchCars(std::vector<CarDescriptor>& existing, const std::vector<CarDescriptor>& current) {
	for (auto &ex : existing) {
		ex.isMatchFound = false;
		ex.predictNextPosition();
	}
	for (auto &cur : current) {
		size_t indexOfLeastDistance = 0;
		double leastDistance = std::numeric_limits<double>::max();
		for (size_t i = 0; i < existing.size(); ++i) {
			if (existing[i].isTracked) {
				double v = distance(cur.centerPositions.back(), existing[i].predictedNextPos);
				if (v < leastDistance) {
					leastDistance = v;
					indexOfLeastDistance = i;
				}
			}
		}
		if (leastDistance < cur.diagonalSize * 0.5) {
			existing[indexOfLeastDistance].assign(cur);
		}
		else {
			existing.push_back(std::move(cur));
			existing.back().isMatchFound = true;
		}
	}
	for (auto &ex : existing) {
		if (!ex.isMatchFound)
			ex.numFramesWithoutMatch++;
		ex.isTracked = ex.numFramesWithoutMatch < 5;
	}
}

#ifdef SHOW_STEPS

void show(const cv::Size& imageSize, const std::vector<std::vector<cv::Point>>& contours, const std::string& title) {
	cv::Mat image(imageSize, CV_8UC3, BLACK);
	cv::drawContours(image, contours, -1, WHITE, -1);
	cv::imshow(title, image);
}

void show(const cv::Size& imageSize, const std::vector<CarDescriptor>& cars, const std::string& title) {
	cv::Mat image(imageSize, CV_8UC3, BLACK);
	std::vector<std::vector<cv::Point>> contours;
	for (auto &car : cars)
		if (car.isTracked == true)
			contours.push_back(car.contour);
	cv::drawContours(image, contours, -1, WHITE, -1);
	cv::imshow(title, image);
}

#endif

bool checkIfLineCrossed(std::vector<CarDescriptor>& cars, int& horizontalLinePos, int& carsCount) {
	bool carCrossedLine = false;
	for (auto &car : cars) {
		if (car.isTracked && car.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)car.centerPositions.size() - 2;
			int currFrameIndex = (int)car.centerPositions.size() - 1;
			// movement down
			if (horizontalLinePos < car.centerPositions[currFrameIndex].y && car.centerPositions[prevFrameIndex].y <= horizontalLinePos) {
				carsCount++;
				carCrossedLine = true;
			}
		}
	}
	return carCrossedLine;
}

void drawCarsInfo(std::vector<CarDescriptor>& cars, cv::Mat& image) {
	for (size_t i = 0; i < cars.size(); i++) {
		auto &car = cars[i];
		if (car.isTracked) {
			cv::rectangle(image, car.boundingRect, RED, 2);
			double fontScale = car.diagonalSize / 120.0;
			int fontThickness = (int)std::round(fontScale * 1.0);
			cv::putText(image, std::to_string(i), car.centerPositions.back(), CV_FONT_HERSHEY_SIMPLEX, fontScale, GREEN, fontThickness);
		}
	}
}

void drawCarsCount(int &carCount, cv::Mat& image) {
	double fontScale = (image.rows * image.cols) / 300000.0;
	int fontThickness = (int)std::round(fontScale * 1.5);
	cv::Size textSize = cv::getTextSize(std::to_string(carCount), CV_FONT_HERSHEY_SIMPLEX, fontScale, fontThickness, 0);
	cv::Point textPos(
		image.cols - 1 - (int)((double)textSize.width * 1.25),
		image.rows - 1 - (int)((double)textSize.height * 1.25)
	);
	cv::putText(image, std::to_string(carCount), textPos, CV_FONT_HERSHEY_SIMPLEX, fontScale, YELLOW, fontThickness);
}

DetectFilter::DetectFilter(QObject* parent) :
	QObject(parent),
	_dropFrames(true),
	_frameCount(0),
	_carCount(0)
{
	_structuringElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
}

void DetectFilter::setSegments(const QVector<QLineF>& segments) {
	//
}

void DetectFilter::process(cv::Mat currentFrame) {
	_frameCount++;
	if (_prevFrame.empty()) {
		_prevFrame = currentFrame;
		return;
	}

	// explicit parameters (temporary)
	int lineHPos = (int)std::round((double)currentFrame.rows * 0.3);
	cv::Point crossingLine[2];
	crossingLine[0] = cv::Point(0, lineHPos);
	crossingLine[1] = cv::Point(currentFrame.cols - 1, lineHPos);
	///
	_currentFrameCars.clear();

	cv::Mat prevFrameCopy, curFrameCopy, imgDifference, imgThresh;
	cv::cvtColor(currentFrame, curFrameCopy, CV_BGR2GRAY);
	cv::GaussianBlur(curFrameCopy, curFrameCopy, cv::Size(7,7), 0);
	cv::cvtColor(_prevFrame, prevFrameCopy, CV_BGR2GRAY);
	cv::GaussianBlur(prevFrameCopy, prevFrameCopy, cv::Size(7,7), 0);
	cv::absdiff(prevFrameCopy, curFrameCopy, imgDifference);
	cv::threshold(imgDifference, imgThresh, 30, 255.0, CV_THRESH_BINARY);
	for (int i = 0; i < 3; i++) {
		cv::dilate(imgThresh, imgThresh, _structuringElement);
		cv::dilate(imgThresh, imgThresh, _structuringElement);
		cv::erode(imgThresh, imgThresh, _structuringElement);
	}
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(imgThresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
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

	// prepare visualization
	auto result = currentFrame.clone();
	drawCarsInfo(_cars, result);
	bool crossed = checkIfLineCrossed(_cars, lineHPos, _carCount);
	if (crossed)
		cv::line(result, crossingLine[0], crossingLine[1], GREEN, 2);
	else
		cv::line(result, crossingLine[0], crossingLine[1], RED, 2);
	drawCarsCount(_carCount, result);
	cv::resize(result, result, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
	cv::cvtColor(result, result, CV_BGR2RGB);
	QImage image(result.data, result.cols, result.rows, (int)result.step,
		QImage::Format_RGB888, &matDeleter, new cv::Mat(result));
	emit imageReady(image);

	_prevFrame = currentFrame;
}
