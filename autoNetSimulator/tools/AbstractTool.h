#ifndef ABSTRACTTOOL_H
#define ABSTRACTTOOL_H

#include <QObject>
#include <QCursor>

class QPainter;
class QMouseEvent;

/**
 * 基础类：用于实现工具如ScaleTool，originTool等
 * 工具可以通过俩种方法接收鼠标事件；
 * The clicked() function gets called when the left button is pressed and released.
 *
 * For more complex interaction, the mousePressEvent()/mouseMoveEvent()/mouseReleaseEvent() functions can be overridden.
 * When a mouse button is pressed, mousePressEvent is called. If it returns true, then tool grabs the mouse interaction,
 * and will receive mouse events until the button is released. Otherwise the scene will be handling mouse events,
 * and mouseMoveEvent()/mouseReleaseEvent() won't be called.
 */

class AbstractTool : public QObject {
    Q_OBJECT

public:
    explicit AbstractTool(QObject *parent = 0) : QObject(parent) {}
    virtual ~AbstractTool() = 0;

    virtual QCursor cursor() {
        return QCursor();
    }

    /**
     * Draw the tool on the scene
     * @param painter the painting context
     * @param rect the area which should be repainted
     * @param cursor the current position of the mouse, in scene coordinates
     */
    virtual void draw(QPainter *painter, const QRectF &rect, const QPointF &cursor) {
        Q_UNUSED(painter)
        Q_UNUSED(rect)
        Q_UNUSED(cursor)
    }

    /**
     * Handle mouse click events.
     * Called when a complete mouse click has completed.
     * @param p the event's position, in scene coordinates
     */
    virtual void clicked(const QPointF &p) {
        Q_UNUSED(p)
    }

    /**
     * Handle mouse press events.
     * If true is returned, the tool will obtain the mouse grab, and receive future mouse events, until the mouse button is released.
     * The default implementation returns false.
     * @param p the event's position, in scene coordinates
     * @return true if the tool wants to grab the mouse, false otherwise
     */
    virtual bool mousePressEvent(const QPointF &p) {
        Q_UNUSED(p)
        return false;
    }

    /**
     * Handle mouse move events.
     * @param p the event's position, in scene coordinates
     */
    virtual void mouseMoveEvent(const QPointF &p) {
        Q_UNUSED(p)
    }

    /**
     * Handle mouse release events.
     * @param p the event's position, in scene coordinates
     */
    virtual void mouseReleaseEvent(const QPointF &p) {
        Q_UNUSED(p)
    }

public slots:
    /**
     * Handle tool cancelation.
     * This function is called when the user right-clicks, or when ESC is pressed.
     * The default implementation emits the done() signal.
     */
    virtual void cancel() {
        emit done(false);
    }

signals:
    /**
     * Signal the tool completion.
     * @param succesful true if the action was succesful.
     */
    void done(bool succesful = true);

};

inline AbstractTool::~AbstractTool() {}

#endif // ABSTRACTTOOL_H
