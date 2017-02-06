#pragma once

#include <QDialog>
#include <QMutex>

class ImageViewer : public QWidget {
	Q_OBJECT
private:
	QMutex _mutex;
	QImage _image;
	void paintEvent(QPaintEvent* ev) override;
public:
	ImageViewer(QWidget* parent = 0);
	Q_SLOT void setImage(const QImage& image);
};
