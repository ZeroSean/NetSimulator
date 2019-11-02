#ifndef TAGTOOL_H
#define TAGTOOL_H

#include "AbstractTool.h"

/**
 * 用来设置tag的坐标
 * It reacts to the clicked() event.
 * When clicked() is called, the origin is calculated based on the click position, and the tool finishes right away.
 */

class TagTool : public AbstractTool {
    Q_OBJECT

public:
    explicit TagTool(QObject *parent = 0);
    virtual ~TagTool() {}

    virtual QCursor cursor();

    virtual void draw(QPainter *painter, const QRectF &rect, const QPointF &cursor);

    virtual void clicked(const QPointF &scenePos);

    void updatePosition(const QPointF &scenePos);

public slots:
    virtual void cancel();

public:
    enum State {
        NoClicked,
        Clicked
    };

    State _state;

};


#endif // TAGTOOL_H
