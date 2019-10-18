#ifndef ORIGINTOOL_H
#define ORIGINTOOL_H

#include "AbstractTool.h"

/**
 * 用来设置floorplan的原点
 * It reacts to the clicked() event.
 * When clicked() is called, the origin is calculated based on the click position, and the tool finishes right away.
 */

class OriginTool : public AbstractTool {
    Q_OBJECT

public:
    explicit OriginTool(QObject *parent = 0);
    virtual ~OriginTool() {}

    virtual QCursor cursor();

    /**
      * Handle mouse clicks.
      * This function calculate the new origin point based on scenePos, and updates it with the new value.
      * The tool then emits the done() signal.
      *
      * @param scenePos the position of the click in scene coordinates.
      */
    virtual void clicked(const QPointF &scenePos);

signals:

public slots:

};

#endif // ORIGINTOOL_H
