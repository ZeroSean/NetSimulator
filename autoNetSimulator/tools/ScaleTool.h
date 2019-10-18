#ifndef SCALETOOL_H
#define SCALETOOL_H

#include "AbstractTool.h"

#include <QPointF>

/**
 * 用来改变平面图的比例变换
 * 用户通过连续左击俩个点得到scale比例（俩个点的距离），
 */

class ScaleTool : public AbstractTool {
    Q_OBJECT

public:
    /**
     * 指明本工具针对哪个维度操作
     */
    enum Axis {
        XAxis,      //x 维度
        YAxis       //y 维度
    };

    explicit ScaleTool(Axis a, QObject *parent = 0);
    virtual ~ScaleTool() {}

    virtual QCursor cursor();

    /**
     * 在场景中绘制，一旦左击了第一个点，该函数就开始在俩点间绘制线段（第一个点和光标点）
     * @param painter the painting context
     * @param rect the area which should be repainted
     * @param cursor the current position of the mouse, in scene coordinates
     */
    virtual void draw(QPainter *painter, const QRectF &rect, const QPointF &cursor);

    /**
     * 鼠标点击控制
     * @param scenePos the position of the click in scene coordinates.
     */
    virtual void clicked(const QPointF &scenePos);

public slots:
    virtual void cancel();

private:
    Axis _axis;

    QPointF _first;

    /**
     * 左击状态
     */
    enum State {
        FirstPoint,     //等待左击第一个点
        SecondPoint     //等待左击第二个点
    };

    State _state;

};

#endif // SCALETOOL_H
