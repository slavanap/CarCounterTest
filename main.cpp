#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <algorithm>
#include <iterator>
#include <vector>

#define SHOW_STEPS 0

#include "main.h"

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


CarFilterRunnable::CarFilterRunnable(CarFilter *filter) {

}

CarFilterRunnable::~CarFilterRunnable() {

}

QVideoFrame CarFilterRunnable::run(QVideoFrame* input, const QVideoSurfaceFormat& surfaceFormat, RunFlags flags) override {
	return *input;
}

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




#if 0
CarFilterRunnable::CarFilterRunnable(CLFilter *filter) :
	m_filter(filter),
	m_tempTexture(0),
	m_outTexture(0),
	m_lastInputTexture(0),
	m_clContext(0),
	m_clQueue(0),
	m_clProgram(0),
	m_clKernel(0)
{
	m_clImage[0] = m_clImage[1] = 0;

	// Set up OpenCL.
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
	cl_uint n;
	cl_int err = clGetPlatformIDs(0, 0, &n);
	if (err != CL_SUCCESS) {
		qWarning("Failed to get platform ID count (error %d)", err);
		if (err == -1001) {
			qDebug("Could not find OpenCL implementation. ICD missing?"
#ifdef Q_OS_LINUX
				   " Check /etc/OpenCL/vendors."
#endif
				);
		}
		return;
	}
	if (n == 0) {
		qWarning("No OpenCL platform found");
		return;
	}
	QVector<cl_platform_id> platformIds;
	platformIds.resize(n);
	if (clGetPlatformIDs(n, platformIds.data(), 0) != CL_SUCCESS) {
		qWarning("Failed to get platform IDs");
		return;
	}
	cl_platform_id platform = platformIds[0];
	const char *vendor = (const char *) f->glGetString(GL_VENDOR);
	qDebug("GL_VENDOR: %s", vendor);
	const bool isNV = vendor && strstr(vendor, "NVIDIA");
	const bool isIntel = vendor && strstr(vendor, "Intel");
	const bool isAMD = vendor && strstr(vendor, "ATI");
	qDebug("Found %u OpenCL platforms:", n);
	for (cl_uint i = 0; i < n; ++i) {
		QByteArray name;
		name.resize(1024);
		clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, name.size(), name.data(), 0);
		qDebug("Platform %p: %s", platformIds[i], name.constData());
		// Running with an OpenCL platform without GPU support is not going
		// to cut it. In practice we want the platform for the GPU which we
		// are using with OpenGL.
		if (isNV && name.contains(QByteArrayLiteral("NVIDIA")))
			platform = platformIds[i];
		else if (isIntel && name.contains(QByteArrayLiteral("Intel")))
			platform = platformIds[i];
		else if (isAMD && name.contains(QByteArrayLiteral("AMD")))
			platform = platformIds[i];
	}
	qDebug("Using platform %p", platform);

	// Set up the context with OpenCL/OpenGL interop.
#if defined (Q_OS_OSX)
	cl_context_properties contextProps[] = { CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
											 (cl_context_properties) CGLGetShareGroup(CGLGetCurrentContext()),
											 0 };
#elif defined(Q_OS_WIN)
	cl_context_properties contextProps[] = { CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
											 CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
											 CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
											 0 };
#elif defined(Q_OS_LINUX)
	QVariant nativeGLXHandle = QOpenGLContext::currentContext()->nativeHandle();
	QGLXNativeContext nativeGLXContext;
	if (!nativeGLXHandle.isNull() && nativeGLXHandle.canConvert<QGLXNativeContext>())
		nativeGLXContext = nativeGLXHandle.value<QGLXNativeContext>();
	else
		qWarning("Failed to get the underlying GLX context from the current QOpenGLContext");
	cl_context_properties contextProps[] = { CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
											 CL_GL_CONTEXT_KHR, (cl_context_properties) nativeGLXContext.context(),
											 CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
											 0 };
#endif

	m_clContext = clCreateContextFromType(contextProps, CL_DEVICE_TYPE_GPU, 0, 0, &err);
	if (!m_clContext) {
		qWarning("Failed to create OpenCL context: %d", err);
		return;
	}

	// Get the GPU device id
#if defined(Q_OS_OSX)
	// On OS X, get the "online" device/GPU. This is required for OpenCL/OpenGL context sharing.
	err = clGetGLContextInfoAPPLE(m_clContext, CGLGetCurrentContext(),
								  CL_CGL_DEVICE_FOR_CURRENT_VIRTUAL_SCREEN_APPLE,
								  sizeof(cl_device_id), &m_clDeviceId, 0);
	if (err != CL_SUCCESS) {
		qWarning("Failed to get OpenCL device for current screen: %d", err);
		return;
	}
#else
	clGetGLContextInfoKHR_fn getGLContextInfo = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
	if (!getGLContextInfo || getGLContextInfo(contextProps, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR,
											  sizeof(cl_device_id), &m_clDeviceId, 0) != CL_SUCCESS) {
		err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &m_clDeviceId, 0);
		if (err != CL_SUCCESS) {
			qWarning("Failed to get OpenCL device: %d", err);
			return;
		}
	}
#endif

	m_clQueue = clCreateCommandQueue(m_clContext, m_clDeviceId, 0, &err);
	if (!m_clQueue) {
		qWarning("Failed to create OpenCL command queue: %d", err);
		return;
	}
	// Build the program.
	m_clProgram = clCreateProgramWithSource(m_clContext, 1, &openclSrc, 0, &err);
	if (!m_clProgram) {
		qWarning("Failed to create OpenCL program: %d", err);
		return;
	}
	if (clBuildProgram(m_clProgram, 1, &m_clDeviceId, 0, 0, 0) != CL_SUCCESS) {
		qWarning("Failed to build OpenCL program");
		QByteArray log;
		log.resize(2048);
		clGetProgramBuildInfo(m_clProgram, m_clDeviceId, CL_PROGRAM_BUILD_LOG, log.size(), log.data(), 0);
		qDebug("Build log: %s", log.constData());
		return;
	}
	m_clKernel = clCreateKernel(m_clProgram, "Emboss", &err);
	if (!m_clKernel) {
		qWarning("Failed to create emboss OpenCL kernel: %d", err);
		return;
	}
}

CLFilterRunnable::~CLFilterRunnable()
{
	releaseTextures();
	if (m_clKernel)
		clReleaseKernel(m_clKernel);
	if (m_clProgram)
		clReleaseProgram(m_clProgram);
	if (m_clQueue)
		clReleaseCommandQueue(m_clQueue);
	if (m_clContext)
		clReleaseContext(m_clContext);
}

void CLFilterRunnable::releaseTextures()
{
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
	if (m_tempTexture)
		f->glDeleteTextures(1, &m_tempTexture);
	if (m_outTexture)
		f->glDeleteTextures(1, &m_outTexture);
	m_tempTexture = m_outTexture = m_lastInputTexture = 0;
	if (m_clImage[0])
		clReleaseMemObject(m_clImage[0]);
	if (m_clImage[1])
		clReleaseMemObject(m_clImage[1]);
	m_clImage[0] = m_clImage[1] = 0;
}

uint CLFilterRunnable::newTexture()
{
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
	GLuint texture;
	f->glGenTextures(1, &texture);
	f->glBindTexture(GL_TEXTURE_2D, texture);
	f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.width(), m_size.height(),
					0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	return texture;
}

QVideoFrame CLFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags)
{
	Q_UNUSED(surfaceFormat);
	Q_UNUSED(flags);

	// This example supports RGB data only, either in system memory (typical with cameras on all
	// platforms) or as an OpenGL texture (e.g. video playback on OS X).
	// The latter is the fast path where everything happens on GPU. THe former involves a texture upload.

	if (!input->isValid()
			|| (input->handleType() != QAbstractVideoBuffer::NoHandle
				&& input->handleType() != QAbstractVideoBuffer::GLTextureHandle)) {
		qWarning("Invalid input format");
		return *input;
	}

	if (input->pixelFormat() == QVideoFrame::Format_YUV420P
			|| input->pixelFormat() == QVideoFrame::Format_YV12) {
		qWarning("YUV data is not supported");
		return *input;
	}

	if (m_size != input->size()) {
		releaseTextures();
		m_size = input->size();
	}

	// Create a texture from the image data.
	QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
	GLuint texture;
	if (input->handleType() == QAbstractVideoBuffer::NoHandle) {
		// Upload.
		if (m_tempTexture)
			f->glBindTexture(GL_TEXTURE_2D, m_tempTexture);
		else
			m_tempTexture = newTexture();
		input->map(QAbstractVideoBuffer::ReadOnly);
		// glTexImage2D only once and use TexSubImage later on. This avoids the need
		// to recreate the CL image object on every frame.
		f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_size.width(), m_size.height(),
						   GL_RGBA, GL_UNSIGNED_BYTE, input->bits());
		input->unmap();
		texture = m_tempTexture;
	} else {
		// Already an OpenGL texture.
		texture = input->handle().toUInt();
		f->glBindTexture(GL_TEXTURE_2D, texture);
		// Unlike on the other branch, the input texture may change, so m_clImage[0] may need to be recreated.
		if (m_lastInputTexture && m_lastInputTexture != texture && m_clImage[0]) {
			clReleaseMemObject(m_clImage[0]);
			m_clImage[0] = 0;
		}
		m_lastInputTexture = texture;
	}

	// OpenCL image objects cannot be read and written at the same time. So use
	// a separate texture for the result.
	if (!m_outTexture)
		m_outTexture = newTexture();

	// Create the image objects if not yet done.
	cl_int err;
	if (!m_clImage[0]) {
		m_clImage[0] = clCreateFromGLTexture2D(m_clContext, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, texture, &err);
		if (!m_clImage[0]) {
			qWarning("Failed to create OpenGL image object from OpenGL texture: %d", err);
			return *input;
		}
		cl_image_format fmt;
		if (clGetImageInfo(m_clImage[0], CL_IMAGE_FORMAT, sizeof(fmt), &fmt, 0) != CL_SUCCESS) {
			qWarning("Failed to query image format");
			return *input;
		}
		if (fmt.image_channel_order != CL_RGBA)
			qWarning("OpenCL image is not RGBA, expect errors");
	}
	if (!m_clImage[1]) {
		m_clImage[1] = clCreateFromGLTexture2D(m_clContext, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, m_outTexture, &err);
		if (!m_clImage[1]) {
			qWarning("Failed to create output OpenGL image object from OpenGL texture: %d", err);
			return *input;
		}
	}

	// We are all set. Queue acquiring the image objects.
	f->glFinish();
	clEnqueueAcquireGLObjects(m_clQueue, 2, m_clImage, 0, 0, 0);

	// Set up the kernel arguments.
	clSetKernelArg(m_clKernel, 0, sizeof(cl_mem), &m_clImage[0]);
	clSetKernelArg(m_clKernel, 1, sizeof(cl_mem), &m_clImage[1]);
	// Accessing dynamic properties on the filter element is simple:
	cl_float factor = m_filter->factor();
	clSetKernelArg(m_clKernel, 2, sizeof(cl_float), &factor);

	// And queue the kernel.
	const size_t workSize[] = { size_t(m_size.width()), size_t(m_size.height()) };
	err = clEnqueueNDRangeKernel(m_clQueue, m_clKernel, 2, 0, workSize, 0, 0, 0, 0);
	if (err != CL_SUCCESS)
		qWarning("Failed to enqueue kernel: %d", err);

	// Return the texture from our output image object.
	// We return a texture even when the original video frame had pixel data in system memory.
	// Qt Multimedia is smart enough to handle this. Once the data is on the GPU, it stays there. No readbacks, no copies.
	clEnqueueReleaseGLObjects(m_clQueue, 2, m_clImage, 0, 0, 0);
	clFinish(m_clQueue);
	return frameFromTexture(m_outTexture, m_size, input->pixelFormat());
}

#endif


int main(int argc, char* argv[]) {
//#ifdef Q_OS_WIN // avoid ANGLE on Windows
	//QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
//#endif
	QGuiApplication app(argc, argv);
	qmlRegisterType<CarFilter>("carfilter", 1, 0, "CarFilter");

	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	return app.exec();
}
