#pragma once
#include <QGraphicsScene>
#include <QMainWindow>
#include <QThread>

#include "GraphicsItemPolyline.h"
#include "processing.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	Q_SLOT void setImage(const QImage& image);

private:
	Ui::MainWindow *ui;
	QGraphicsPixmapItem *pixmapItem;
	GraphicsItemPolyline *polylineItem;

	QThread captureThread, detectThread;
	DetectFilter detectFilter;

};
