#include "ScaleTool.h"

#include "DisplayApplication.h"
#include "mainwindow.h"
#include "ViewSettings.h"

#include <QCursor>
#include <QInputDialog>
#include <QPainter>
#include <QDebug>

ScaleTool::ScaleTool(Axis a, QObject *parent) :
    AbstractTool(parent),
    _axis(a),
    _state(FirstPoint)
{

}

QCursor ScaleTool::cursor() {
    return Qt::CrossCursor;
}

void ScaleTool::draw(QPainter *painter, const QRectF &rect, const QPointF &cursor) {
    Q_UNUSED(rect)

    if(_state == SecondPoint) {
        painter->save();
        painter->setPen(QPen(QBrush(Qt::black), 0));
        painter->drawLine(_first, cursor);
        painter->restore();
    }
}

void ScaleTool::clicked(const QPointF &scenePos) {
    ViewSettings *vs = DisplayApplication::viewSettings();

    if(_state == FirstPoint) {
        _first = scenePos;
        _state = SecondPoint;
    } else if(_state == SecondPoint) {
        double p1 = (_axis == XAxis) ? _first.x() : _first.y();
        double p2 = (_axis == XAxis) ? scenePos.x() : scenePos.y();

        bool ok = false;
        double distance = QInputDialog::getDouble(DisplayApplication::mainWindow(),
                                                  "Distance", "Distance between the two points",
                                                  qAbs(p2 - p1), 0, 1000, 3, &ok);
        if(ok) {
            if(_axis == XAxis) {
                vs->setFloorplanXScale(distance);
            } else {
                vs->setFloorplanYScale(distance);
            }
        }
        emit done();
    }
}


void ScaleTool::cancel() {
    emit done(false);

    _first = QPointF();
    _state = FirstPoint;
}

