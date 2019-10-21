#include "MinimapView.h"

#include "DisplayApplication.h"
#include "ViewSettings.h"
#include "GraphicsView.h"

#include <QMouseEvent>
#include <qmath.h>

MinimapView::MinimapView(QWidget *parent) :
    QGraphicsView(parent),
    _scene(new QGraphicsScene(this))
{
    setScene(_scene);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setDragMode(QGraphicsView::NoDrag);

    scale(1, -1);

    DisplayApplication::connectReady(this, "onReady()");
}

void MinimapView::onReady() {
    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(floorplanChanged()), this, SLOT(floorplanChanged()));
    QObject::connect(DisplayApplication::graphicsView(), SIGNAL(visibleRectChanged(QRectF)), this, SLOT(visibleRectChanged()));
}

void MinimapView::floorplanChanged() {
    ViewSettings *vs = DisplayApplication::viewSettings();
    QRectF sceneRect = vs->floorplanTransform()
            .map(QRectF(0, 0, vs->floorplanPixmap().width(), vs->floorplanPixmap().height()))
            .boundingRect();

    _scene->setSceneRect(sceneRect);
    this->fitInView(sceneRect, Qt::KeepAspectRatio);
}

void MinimapView::visibleRectChanged() {
    _scene->update();
}

void MinimapView::drawForeground(QPainter *painter, const QRectF &rect) {
    Q_UNUSED(rect)
    const QPixmap &pm = DisplayApplication::viewSettings()->floorplanPixmap();
    if(!pm.isNull()) {
        painter->save();
        painter->setTransform(DisplayApplication::viewSettings()->floorplanTransform(), true);
        painter->setPen(QPen(QBrush(Qt::black), 1));
        painter->drawPixmap(0, 0, pm);
        painter->restore();

        painter->setPen(QPen(QBrush(Qt::red), 0.1));
        painter->drawRect(DisplayApplication::graphicsView()->visibleRect());
    }
}

void MinimapView::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    ViewSettings *vs = DisplayApplication::viewSettings();
    QRectF sceneRect = vs->floorplanTransform()
            .map(QRectF(0, 0, vs->floorplanPixmap().width(), vs->floorplanPixmap().height()))
            .boundingRect();
    this->fitInView(sceneRect, Qt::KeepAspectRatio);
}

void MinimapView::mousePressEvent(QMouseEvent *event) {
    QRectF visibleRect = DisplayApplication::graphicsView()->visibleRect();
    visibleRect.moveCenter(mapToScene(event->pos()));
    DisplayApplication::graphicsView()->setVisibleRect(visibleRect);
}

void MinimapView::mouseMoveEvent(QMouseEvent *event) {
    QRectF visibleRect = DisplayApplication::graphicsView()->visibleRect();
    visibleRect.moveCenter(mapToScene(event->pos()));
    DisplayApplication::graphicsView()->setVisibleRect(visibleRect);
}

void MinimapView::wheelEvent(QWheelEvent *event) {
    qreal s = pow((double)2.0, event->delta() / 360.0);
    DisplayApplication::graphicsView()->scaleView(s, s);
}

