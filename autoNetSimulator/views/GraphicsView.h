#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>

class QGestureEvent;
class AbstractTool;

/**
 * The GraphicsView class draws the scene and provides user interaction using the mouse.
 *
 * @par User Interaction
 * @parblock
 * User interaction is quite tricky as we have to handle many different interaction based on very little mouse events.
 * - Scene interaction (Selecting anchors, mvoing them, ...) - This is handled by QGraphicsScene and GraphicsDataItem.
 * - Unselecting all anchors by clicking on the background.
 * - Zooming using the scroll wheel
 * - Panning by dragging
 * - Contextual menu by right-click
 * - Canceling the current tool by right-click
 * - Starting a RubberBandTool on Ctrl+Drag
 * - Tools listening to AbstractTool::clicked() events
 * - Tools listening to AbstractTool::mousePressEvent() events
 *
 * Most of them are handled by the mousePressEvent()/mouseMoveEvent()/mouseReleaseEvent() cycle.
 *
 * During mousePressEvent(), we find the suited action, and decide of a MouseContext based on that.
 * The MouseContext is kept until mouseReleaseEvent()
 * @endparblock
 *
 * @par Visible Rectangle
 * @parblock
 * The GraphicsView keeps track of the visible rectangle, in scene coordinates. This rectangle will always be visible on the screen.
 * Initially, the visible rectangle is the square from (0,0) to (1, 1) \n
 * The visible rectangle can be transformed using translateView() or scaleView() or changed using setVisibleRect().
 * Whenever the visible rectangle changes, for any reason, the visibleRectChanged() signal is called.
 * @endparblock
 *
 * @par Tool
 * Tools allow simple interaction inside the scene. A new tool can be set using setTool(). The tool then remains active until it's AbstractTool::done() signal is emitted. \n
 * When ESC button or right click is pressed, the view attempts to cancel the tool by calling AbstractTool::cancel().
 * @see AbstractTool
 */

class GraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphicsView(QWidget *parent = 0);
    virtual ~GraphicsView();

    /**
     * 可视矩形平移
     */
    void translateView(qreal dx, qreal dy);

    /**
     * 缩放可视矩形
     */
    void scaleView(qreal sx, qreal sy);

    /**
     * 缩放可视矩形,其中center中心保持在可视矩形中心
     */
    void scaleView(qreal sx, qreal sy, QPointF center);

    /**
     * 更改当前的可视矩形
     */
    void setVisibleRect(const QRectF &visibleRect);
    QRectF visibleRect() const;

    /**
     * 为视图分配新的实例工具
     */
    void setTool(AbstractTool *tool);

signals:
    void visibleRectChanged(const QRectF &rect);

protected slots:
    void onReady();
    void floorplanChanged();

    void toolDone();
    void toolDestroyed();

    void centerRect(const QRectF &visibleRect);
    void centerAt(double x, double y);

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void drawForeground(QPainter *painter, const QRectF &rect);
    virtual void drawBackground(QPainter *painter, const QRectF &rect);

    void drawGrid(QPainter *painter, const QRectF &rect);
    void drawOrigin(QPainter *painter);
    void drawFloorplan(QPainter *painter, const QRectF &rect);

private:
    QPoint _lastMouse;
    QRectF _visibleRect;
    AbstractTool *_tool;
    bool _ignoreContextMenu;

    /**
     * 标识鼠标交互的可能状态
     */
    enum MouseContext{
        DefaultMouseContext,    //简单交互
        PanningMouseContext,    //拖拽平移
        SceneMouseContext,      //鼠标事件由场景和item处理
        ToolMouseContext        //工具接收鼠标事件
    };
    MouseContext _mouseContext;
};

#endif // GRAPHICSVIEW_H
