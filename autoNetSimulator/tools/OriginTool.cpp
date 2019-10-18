#include "OriginTool.h"

#include "ViewSettings.h"
#include "DisplayApplication.h"
#include "GraphicsView.h"

#include <QCursor>

OriginTool::OriginTool(QObject *parent) :
    AbstractTool(parent)
{
}

QCursor OriginTool::cursor()
{
    return Qt::CrossCursor;
}

void OriginTool::clicked(const QPointF &scenePos)
{
    ViewSettings *vs = DisplayApplication::viewSettings();
    QPointF b = vs->floorplanTransform().inverted().map(scenePos);
    vs->setFloorplanXOffset(b.x());
    vs->setFloorplanYOffset(b.y());

    DisplayApplication::graphicsView()->translateView(-scenePos.x(), -scenePos.y());

    emit done();
}
