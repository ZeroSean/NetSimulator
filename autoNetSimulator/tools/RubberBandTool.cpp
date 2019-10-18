#include "RubberBandTool.h"

#include "DisplayApplication.h"
#include "GraphicsView.h"
#include "mainwindow.h"

#include <QItemSelectionModel>
#include <QCursor>
#include <QPainter>
#include <QMouseEvent>
#include <QCommonStyle>

RubberBandTool::RubberBandTool(QObject *parent = 0) :
    AbstractTool(parent)
{

}

void RubberBandTool::draw(QPainter *painter, const QRect &rect, const QPointF &cursor) {
    double f = 100.0;

    painter->save();
    painter->setClipRect(rect); //启动剪切
    painter->scale(1.0 / f, 1.0 / f);

    QRectF area = QRectF(_start * f, cursor * f).normalized();

    QStyleOptionRubberBand opt;
    opt.rect = area.toAlignedRect();
    QApplication::style()->drawControl(QStyle::CE_RubberBand, &opt, painter);

    painter->restore();
}

bool RubberBandTool::mousePressEvent(const QPointF &scenePos) {
    _previousItems.clear();
    _start = scenePos;
    return true;
}

void RubberBandTool::mouseMoveEvent(const QPointF &scenePos) {
    QRectF area = QRectF(_start, scenePos).normalized();
    QGraphicsScene *scene = DisplayApplication::graphicsView()->scene();

    QSet<QGraphicsItem *> newItems = scene->items(area).toSet();
    QSet<QGraphicsItem *> oldItems = _previousItems;

    oldItems -= newItems;
    newItems -= _previousItems;
    _previousItems = scene->items(area).toSet();
}

void RubberBandTool::mouseReleaseEvent(const QPointF &scenePos)
{
    Q_UNUSED(scenePos)
    emit done();
}

void RubberBandTool::cancel()
{
    AbstractTool::cancel();
    _previousItems.clear();
}
