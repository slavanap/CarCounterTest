#include <QGraphicsPixmapItem>
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "GraphicsItemPolyline.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	_opened(false)
{
	ui->setupUi(this);

	ui->graphicsView->setScene(new QGraphicsScene(this));
	//ui->graphicsView->scene()->setSceneRect(QRectF(0, 0, 640, 480));

#if 0
	auto player = new QMediaPlayer(this);
	QGraphicsVideoItem *item = new QGraphicsVideoItem;
	player->setVideoOutput(item);
	ui->graphicsView->scene()->addItem(item);
	ui->graphicsView->show();
	player->setMedia(QUrl::fromLocalFile("c:\\Users\\Vyacheslav\\Projects\\TestVideo\\day.avi"));
	player->play();
#endif

	connect(ui->actionOpen_File, SIGNAL(triggered()), SLOT(actionFileOpen()));

	pixmapItem = new QGraphicsPixmapItem();
	ui->graphicsView->scene()->addItem(pixmapItem);
	polylineItem = new GraphicsItemPolyline(ui->graphicsView->scene());

	detectFilter.setDropFrames(false);
//	captureThread.start();
	detectThread.start();
//	captureFilter.moveToThread(&captureThread);
	detectFilter.moveToThread(&detectThread);
//	detectFilter.connect(&captureFilter, SIGNAL(matReady(cv::Mat)), SLOT(processFrame(cv::Mat)));
	connect(&detectFilter, SIGNAL(imageReady(QImage)), SLOT(setImage(QImage)));
}

MainWindow::~MainWindow() {
	captureThread.quit();
	detectThread.quit();
	captureThread.wait();
	detectThread.wait();

	delete ui;
	delete pixmapItem;
}

void MainWindow::setImage(const QImage& image) {
	pixmapItem->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::actionFileOpen() {
	if (_opened)
		return;
	auto fileName = QFileDialog::getOpenFileName(this,
		tr("Open Video"), QString(), tr("Video Files (*.*)"));
	if (!fileName.isEmpty()) {
		QMetaObject::invokeMethod(&detectFilter, "startFile", Q_ARG(QString, fileName));
		_opened = true;
	}
}
