#pragma once

#include <QGraphicsItem>
#include <QVector>
#include <QBrush>
#include <QPen>
#include <QMenu>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>

class GraphicsItemPolylinePoint : public QGraphicsObject {
	Q_OBJECT
public:
	explicit GraphicsItemPolylinePoint(const QRectF& rect, const QBrush& brush, QGraphicsItem* parent = nullptr);
	~GraphicsItemPolylinePoint();
	QRectF boundingRect() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
	Q_SIGNAL void deletePoint();
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* ev) override;
private:
	QRectF _rect;
	QBrush _brush;
	QMenu *_menu;
	Q_SLOT void menuDeletePoint();
};

class GraphicsItemPolyline : public QGraphicsObject {
	Q_OBJECT
public:
	explicit GraphicsItemPolyline(QGraphicsScene* scene, QGraphicsItem* parent = nullptr);
	~GraphicsItemPolyline();
	QRectF boundingRect() const override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
	QVector<QLineF> segments() const;
	Q_SIGNAL void segmentsUpdated(const QVector<QLineF>& segments);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
	mutable QMutex _mutex;
	QSizeF _ptSize;
	QBrush _brush;
	QPen _pen;
	QPen _penInverted;
	QMenu *_menu;
	QPointF _menuPoint;
	mutable QRectF _boundRect;

	struct Item {
		QPointF cache;
		GraphicsItemPolylinePoint* object;
		bool invertDirection;
		Item() : invertDirection(false) { }
		Item(const QPointF& point, GraphicsItemPolyline* parent);
	};
	friend struct Item;
	QVector<Item> _points;
	int IndexOfPoint(QObject* sender) const;
	int IndexOfSegment(const QPointF& pos) const;	// returns first of points

	void InsertPoint(int pos, const QPointF& pt);
	bool UpdatePoint(int pos);
	void DeletePoint(int pos);

	Q_SLOT void itemPosChanged();
	Q_SLOT void menuAddPoint();
	Q_SLOT void menuDeletePoint();
	Q_SLOT void menuInvertSegment();
};
