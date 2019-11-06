#include "TagTool.h"

#include "ViewSettings.h"
#include "DisplayApplication.h"
#include "GraphicsView.h"
#include "GraphicsWidget.h"

#include <QCursor>
#include <QDebug>

TagTool::TagTool(QObject *parent) :
    AbstractTool(parent),
    _state(NoClicked)
{
}

QCursor TagTool::cursor()
{
    return Qt::CrossCursor;
}

void TagTool::updatePosition(const QPointF &scenePos) {
    //ViewSettings *vs = DisplayApplication::viewSettings();
    //QPointF b = vs->floorplanTransform().inverted().map(scenePos);

    //qDebug() << b.x() << b.y();

    DisplayApplication::graphicsWidget()->tagConfigChanged(scenePos.x(), scenePos.y());
}

void TagTool::clicked(const QPointF &scenePos)
{
    updatePosition(scenePos);
    _state = Clicked;
}

void TagTool::draw(QPainter *painter, const QRectF &rect, const QPointF &cursor) {
    Q_UNUSED(painter)
    Q_UNUSED(rect)

    if(_state == Clicked) {
        updatePosition(cursor);
    }
}

void TagTool::cancel() {
    _state = NoClicked;

    emit done();
}

