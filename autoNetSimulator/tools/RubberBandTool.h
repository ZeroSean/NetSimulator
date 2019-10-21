#ifndef RUBBERBANDTOOL_H
#define RUBBERBANDTOOL_H

#include "AbstractTool.h"

#include <QPointF>
#include <QSet>

class QGraphicsItem;


/**
 * 用于拖拽出一个矩形区域，选择其中的图形
 * 一旦用户开始拖拽，开始绘制矩形，以及选择图形
 * 拖拽开始使用ctrl + 左击
 */
class RubberBandTool : public AbstractTool {
    Q_OBJECT

public:
    explicit RubberBandTool(QObject *parent = 0);
    virtual ~RubberBandTool() {}

    /**
     * 在场景中绘制矩形
     */
    virtual void draw(QPainter *painter, const QRectF &rect, const QPointF &cursor);

    /**
     * 鼠标点击控制，记录点击点坐标作为矩形区域的起始点，返回true去接收后续子事件
     */
    virtual bool mousePressEvent(const QPointF &scenePos);

    /**
     * 鼠标移动控制，更新矩形区域
     */
    virtual void mouseMoveEvent(const QPointF &scenePos);

    /**
     * 鼠标点击释放控制， 发送done()通知拖拽绘制完成
     */
    virtual void mouseReleaseEvent(const QPointF &scenePos);

signals:

public slots:
    virtual void cancel();

private:
    QPointF _start;
    QSet<QGraphicsItem *> _previousItems;
};

#endif // RUBBERBANDTOOL_H
