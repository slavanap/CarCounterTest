#include <QGraphicsPixmapItem>
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GraphicsItemPolyline.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	connect(ui->actionOpen_File, SIGNAL(triggered()), SLOT(actionFileOpen()));

	ui->graphicsView->setScene(new QGraphicsScene(this));
	pixmapItem.reset(new QGraphicsPixmapItem());
	ui->graphicsView->scene()->addItem(pixmapItem.data());
	filterThread.start();
	filter.moveToThread(&filterThread);

	polylineItem.reset(new GraphicsItemPolyline(ui->graphicsView->scene()));
	filter.setSegments(polylineItem->segments());
	connect(polylineItem.data(), SIGNAL(segmentsUpdated(QVector<QLineF>)), &filter, SLOT(setSegments(QVector<QLineF>)));
	connect(&filter, SIGNAL(newFrame(QImage)), SLOT(setImage(QImage)));

	//QMetaObject::invokeMethod(&filter, "open", Q_ARG(QString, "c:\\Users\\Vyacheslav\\Projects\\TestVideo\\night.avi"));
}

MainWindow::~MainWindow() {
	filterThread.quit();
	filterThread.wait();
	delete ui;
}

void MainWindow::setImage(const QImage& image) {
	pixmapItem->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::actionFileOpen() {
	auto fileName = QFileDialog::getOpenFileName(this,
		tr("Open Video"), QString(), tr("Video Files (*.*)"));
	if (!fileName.isEmpty())
		QMetaObject::invokeMethod(&filter, "open", Q_ARG(QString, fileName));
}
