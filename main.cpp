#include <QApplication>
#include <opencv2/opencv.hpp>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
//	qRegisterMetaType<cv::Mat>();
	qRegisterMetaType<QVector<QLineF>>();
	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	return a.exec();
}
