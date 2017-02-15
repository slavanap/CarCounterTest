#include "GraphicsItemPolyline.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

GraphicsItemPolylinePoint::GraphicsItemPolylinePoint(const QRectF& rect, const QBrush& brush, QGraphicsItem* parent) :
	QGraphicsObject(parent),
	_rect(rect),
	_brush(brush)
{
	_menu = new QMenu();
	auto action = new QAction(this);
	action->setText(tr("Delete"));
	connect(action, SIGNAL(triggered()), this, SLOT(menuDeletePoint()));
	_menu->addAction(action);
}

GraphicsItemPolylinePoint::~GraphicsItemPolylinePoint() {
	delete _menu;
}

void GraphicsItemPolylinePoint::mousePressEvent(QGraphicsSceneMouseEvent* ev) {
	if (ev->button() == Qt::RightButton) {
		_menu->exec(ev->screenPos());
	}
	QGraphicsObject::mousePressEvent(ev);
}

void GraphicsItemPolylinePoint::menuDeletePoint() {
	emit deletePoint();
}

QRectF GraphicsItemPolylinePoint::boundingRect() const {
	return _rect;
}

void GraphicsItemPolylinePoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->fillRect(_rect, _brush);
	painter->drawRect(_rect);
}





GraphicsItemPolyline::Item::Item(const QPointF& point, GraphicsItemPolyline* parent) :
	cache(point),
	invertDirection(false)
{
	QRectF rect(QPointF(-parent->_ptSize.width() / 2, -parent->_ptSize.height() / 2), parent->_ptSize);
	object = new GraphicsItemPolylinePoint(rect, parent->_brush, parent);
	parent->scene()->addItem(object);
	object->setPos(point);
	QObject::connect(object, SIGNAL(xChanged()), parent, SLOT(itemPosChanged()));
	QObject::connect(object, SIGNAL(yChanged()), parent, SLOT(itemPosChanged()));
	QObject::connect(object, SIGNAL(zChanged()), parent, SLOT(itemPosChanged()));
	QObject::connect(object, SIGNAL(deletePoint()), parent, SLOT(menuDeletePoint()));
	object->setFlag(ItemIsMovable);
}

GraphicsItemPolyline::GraphicsItemPolyline(QGraphicsScene* scene, QGraphicsItem* parent) :
	QGraphicsObject(parent),
	_mutex(QMutex::Recursive),
	_brush(Qt::red),
	_pen(Qt::red),
	_penInverted(Qt::blue),
	_ptSize(10, 10)
{
	_menu = new QMenu();
	auto action = new QAction(this);
	action->setText(tr("Add Point"));
	connect(action, SIGNAL(triggered()), this, SLOT(menuAddPoint()));
	_menu->addAction(action);
	action = new QAction(this);
	action->setText(tr("Invert direction"));
	action->setCheckable(true);
	connect(action, SIGNAL(triggered()), this, SLOT(menuInvertSegment()));
	_menu->addAction(action);
	_invertDirectionAction = action;

	scene->addItem(this);

	_pen.setWidth(3);
	_penInverted.setWidth(3);
	setAcceptedMouseButtons(Qt::RightButton);

	InsertPoint(0, QPointF(0, 0));
	InsertPoint(1, QPointF(100, 0));
}

GraphicsItemPolyline::~GraphicsItemPolyline() {
	for (auto &pt : _points)
		scene()->removeItem(pt.object);
	delete _menu;
}

QRectF GraphicsItemPolyline::boundingRect() const {
	QMutexLocker lock(&_mutex);
	for (auto &item : _points) {
		auto rect = item.object->boundingRect();
		auto newCenter = item.object->pos();
		rect.moveCenter(newCenter);
		_boundRect = _boundRect.united(rect);
	}
	return _boundRect;
}

void GraphicsItemPolyline::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QMutexLocker lock(&_mutex);
	Q_UNUSED(option);
	Q_UNUSED(widget);
#if 0
	auto oldPen = painter->pen();
	QPointF lastPoint = _points[0].cache;
	bool inverted = _points[0].invertDirection;
	painter->setPen(inverted ? _penInverted : _pen);
	for (int i = 1; i < _points.size(); ++i) {
		auto &current = _points[i];
		painter->drawLine(QLineF(lastPoint, current.cache));
		if (inverted != current.invertDirection) {
			inverted = current.invertDirection;
			painter->setPen(inverted ? _penInverted : _pen);
		}
		lastPoint = current.cache;
	}
	painter->setPen(oldPen);
#endif
}

QVector<QLineF> GraphicsItemPolyline::segments() const {
	QMutexLocker lock(&_mutex);
	QVector<QLineF> result;
	for (int i = 0; i < _points.size() - 1; ++i) {
		result.push_back(QLineF(_points[i].cache, _points[i + 1].cache));
		if (_points[i].invertDirection)
			result.back().setPoints(result.back().p2(), result.back().p1());
	}
	return result;
}

void GraphicsItemPolyline::mousePressEvent(QGraphicsSceneMouseEvent* ev) {
	if (ev->button() == Qt::RightButton) {
		_menuPoint = ev->pos();
		{
			QMutexLocker lock(&_mutex);
			int pos = IndexOfSegment(_menuPoint);
			auto &point = _points[pos];
			_invertDirectionAction->setChecked(point.invertDirection);
		}
		_menu->exec(ev->screenPos());
	}
	QGraphicsObject::mousePressEvent(ev);
}

void GraphicsItemPolyline::InsertPoint(int pos, const QPointF& pt) {
	QMutexLocker lock(&_mutex);
	_points.insert(pos, Item(pt, this));
	emit segmentsUpdated(segments());
	update();
}

bool GraphicsItemPolyline::UpdatePoint(int pos) {
	QMutexLocker lock(&_mutex);
	auto &point = _points[pos];
	auto newValue = point.object->pos();
	if (point.cache != newValue) {
		point.cache = newValue;
		emit segmentsUpdated(segments());
		return true;
	}
	return false;
}

void GraphicsItemPolyline::DeletePoint(int pos) {
	QMutexLocker lock(&_mutex);
	scene()->removeItem(_points[pos].object);
	_points.removeAt(pos);
	emit segmentsUpdated(segments());
	update();
}



// slots & utility

int GraphicsItemPolyline::IndexOfPoint(QObject* sender) const {
	QMutexLocker lock(&_mutex);
	for (int i = 0; i < _points.size(); ++i)
		if (_points[i].object == sender)
			return i;
	return -1;
}

int GraphicsItemPolyline::IndexOfSegment(const QPointF& p2) const {
	QMutexLocker lock(&_mutex);
	float distance;
	int index = -1;
	for (int i = 0; i < _points.size() - 1; ++i) {
		auto &p1 = _points[i].cache, &b = _points[i + 1].cache;
		QLineF vl1 = QLineF(p1, QPointF(b.x() - p1.x(), b.y() - p1.y()));
		QLineF vl2 = vl1.normalVector();

		QPointF v1(vl1.dx(), vl1.dy()), v2(vl2.dx(), vl2.dy());

		float g = v2.x()*v1.y() - v1.x()*v2.y();
		float c = p1.x()*v1.y() - p1.y()*v1.x();
		float f = p2.x()*v2.y() - p2.y()*v2.x();
		QPointF x(
			(v2.x()*c - v1.x()*f) / g,
			(v2.y()*c - v1.y()*f) / g
		);
		float cur_distance = QLineF(p2, x).length();
		if (index < 0 || distance > cur_distance) {
			index = i;
			distance = cur_distance;
		}
	}
	return index;
}

void GraphicsItemPolyline::itemPosChanged() {
	int pos = IndexOfPoint(sender());
	if (pos >= 0) {
		if (UpdatePoint(pos))
			update();
	}
}

void GraphicsItemPolyline::menuAddPoint() {
	int pos = IndexOfSegment(_menuPoint);
	InsertPoint(pos + 1, _menuPoint);
}

void GraphicsItemPolyline::menuDeletePoint() {
	QMutexLocker lock(&_mutex);
	int pos = IndexOfPoint(sender());
	if (_points.size() > 2)
		DeletePoint(pos);
}

void GraphicsItemPolyline::menuInvertSegment() {
	QMutexLocker lock(&_mutex);
	int pos = IndexOfSegment(_menuPoint);
	auto &point = _points[pos];
	point.invertDirection = !point.invertDirection;
	emit segmentsUpdated(segments());
	update();
}
