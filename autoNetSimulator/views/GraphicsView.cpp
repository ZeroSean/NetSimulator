#include "GraphicsView.h"

#include "DisplayApplication.h"
#include "AbstractTool.h"
#include "ViewSettings.h"
#include "RubberBandTool.h"

#include <QDebug>
#include <QWheelEvent>
#include <QScrollBar>
#include <qmath.h>

GraphicsView::GraphicsView(QWidget *parent) :
    QGraphicsView(parent),
    _tool(NULL),
    _mouseContext(DefaultMouseContext)
{
    setMouseTracking(true);

    //关闭滚动轮
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::NoDrag);
    //设置场景大小和可视矩形
    setVisibleRect(QRectF(-1, -1, 6, 6));
    setSceneRect(-50, -50, 100, 100);

    DisplayApplication::connectReady(this, "onReady()");
}

void GraphicsView::onReady() {
    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(floorplanChanged()), this, SLOT(floorplanChanged()));
}

GraphicsView::~GraphicsView() {

}

void GraphicsView::translateView(qreal dx, qreal dy) {
    setVisibleRect(_visibleRect.translated(dx, dy));
}

void GraphicsView::scaleView(qreal sx, qreal sy) {
    scaleView(sx, sy, _visibleRect.center());
}

void GraphicsView::scaleView(qreal sx, qreal sy, QPointF center) {
    QRectF visibleRect = this->_visibleRect;
    QPointF diff = center - visibleRect.topLeft();

    visibleRect.translate(diff.x(), diff.y());
    visibleRect.setWidth(qBound(1.0, visibleRect.width() * sx, 50.0));
    visibleRect.setHeight(qBound(1.0, visibleRect.height() * sy, 50.0));
    visibleRect.translate(-diff.x() * visibleRect.width() / _visibleRect.width(),
                          -diff.y() * visibleRect.height() / _visibleRect.height());

    setVisibleRect(visibleRect);
}

void GraphicsView::setVisibleRect(const QRectF &visibleRect) {
    this->_visibleRect = visibleRect;
    this->fitInView(this->_visibleRect, Qt::KeepAspectRatio);

    emit visibleRectChanged(visibleRect);
}

QRectF GraphicsView::visibleRect() const {
    return _visibleRect;
}

void GraphicsView::centerRect(const QRectF &visibleRect) {
    //是图形居中显示
    qreal s = 1.206 * 1.2;
    setVisibleRect(visibleRect);
    scaleView(s, s, this->_visibleRect.center());
}

void GraphicsView::centerAt(double x, double y) {
    QRectF visibleRect = this->_visibleRect;
    QPoint p = QPoint(x, y);

    visibleRect.moveCenter(p);
    setVisibleRect(visibleRect);
}

void GraphicsView::mousePressEvent(QMouseEvent *event) {
    event->ignore();

    //如果使用工具，则将事件发给工具，否则发给父类，其将分发给场景中的item
    if(_tool) {
        if(_tool->mousePressEvent(mapToScene(event->pos()))) {
            event->accept();
            _mouseContext = ToolMouseContext;
        }
    } else {
        QGraphicsView::mousePressEvent(event);
        if(event->isAccepted()) {
            _mouseContext = SceneMouseContext;
        }
    }

    if(!event->isAccepted()) {
        //工具和场景都没有接收它
        if(event->button() == Qt::LeftButton
                && event->modifiers() & Qt::ControlModifier) {
            AbstractTool *t = new RubberBandTool(this);
            QObject::connect(t, SIGNAL(done(bool)), t, SLOT(deleteLater()));
            setTool(t);
            if(_tool->mousePressEvent(mapToScene(event->pos()))) {
                event->accept();
                _mouseContext = ToolMouseContext;
            } else {
                delete t;
            }
        } else if(event->button() == Qt::LeftButton) {
            _lastMouse = event->pos();
            event->accept();
        } else if(event->button() == Qt::RightButton && _tool) {
            _tool->cancel();
            event->accept();
            _ignoreContextMenu = true;
        }
    }

    //If someone has captured the right-click event, then we don't show next context menu
    if(event->button() == Qt::RightButton) {
        _ignoreContextMenu = event->isAccepted() && (_mouseContext != SceneMouseContext);
    }
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event) {
    if(_tool && this->scene()) {
        this->scene()->update();
    }

    switch (_mouseContext) {
    case ToolMouseContext:
        if(_tool) {
            _tool->mouseMoveEvent(mapToScene(event->pos()));
        }
        event->accept();
        break;

    case SceneMouseContext:
        QGraphicsView::mouseMoveEvent(event);
        break;

    case PanningMouseContext:
    {
        QPoint mouse = event->pos();
        QPoint mouseDiff = (mouse - _lastMouse);
        _lastMouse = mouse;
        QPointF mappedMouseDiff = mapToScene(0, 0) - mapToScene(mouseDiff);
        translateView(mappedMouseDiff.x(), mappedMouseDiff.y());
        break;
    }

    case DefaultMouseContext:
        if(event->buttons() & Qt::LeftButton) {
            viewport()->setCursor(Qt::ClosedHandCursor);
            _mouseContext = PanningMouseContext;
        }
        break;
    }
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    switch (_mouseContext) {
    case ToolMouseContext:
        if(_tool) {
            _tool->mouseReleaseEvent(mapToScene(event->pos()));
        }
        event->accept();
        break;

    case SceneMouseContext:
        QGraphicsView::mouseReleaseEvent(event);
        break;

    case PanningMouseContext:
        _lastMouse = QPoint();
        viewport()->unsetCursor();
        break;

    case DefaultMouseContext:
        if(event->button() == Qt::LeftButton) {
            if(_tool) {
                _tool->clicked(mapToScene(event->pos()));
            }
        }
        break;
    }
    _mouseContext = DefaultMouseContext;
}

void GraphicsView::contextMenuEvent(QContextMenuEvent *event) {
    if(_ignoreContextMenu && event->reason() == QContextMenuEvent::Mouse) {
        _ignoreContextMenu = false;
        return;
    }

    QGraphicsView::contextMenuEvent(event);
}

void GraphicsView::wheelEvent(QWheelEvent *event) {
    qreal s = 2 - qPow((double)2.0, event->delta() / 360.0);
    scaleView(s, s, mapToScene(event->pos()));
}

void GraphicsView::showEvent(QShowEvent *event) {
    QGraphicsView::showEvent(event);
    this->fitInView(_visibleRect, Qt::KeepAspectRatio);
}

void GraphicsView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    this->fitInView(_visibleRect, Qt::KeepAspectRatio);
}

void GraphicsView::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Escape && _tool) {
        event->accept();
        _tool->cancel();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void GraphicsView::drawOrigin(QPainter *painter) {
    QPointF p(0, 0);
    painter->setPen(QPen(QBrush(Qt::red), 0));
    painter->drawEllipse(p, 0.05, 0.05);
}

void GraphicsView::drawGrid(QPainter *painter, const QRectF &rect) {
    QRectF adjusted = rect.adjusted(-0.025, -0.025, 0.025, 0.025);

    double width = DisplayApplication::viewSettings()->gridWidth();
    double height = DisplayApplication::viewSettings()->gridHeight();

    double left = width * qCeil(adjusted.left() / width);
    double top = height * qCeil(adjusted.top() / height);

    painter->setPen(QPen(QBrush(Qt::lightGray), 0));

    if(width > 0 && height > 0) {
        for(double x = left; x < adjusted.right(); x += width) {
            for(double y = top; y < adjusted.bottom(); y += height) {
                painter->drawLine(QPointF(x, y - 0.025), QPointF(x, y + 0.025));
                painter->drawLine(QPointF(x - 0.025, y), QPointF(x + 0.025, y));
            }
        }
    }
}

void GraphicsView::drawFloorplan(QPainter *painter, const QRectF &rect) {
    Q_UNUSED(rect)

    const QPixmap &pm = DisplayApplication::viewSettings()->floorplanPixmap();
    if(!pm.isNull()) {
        painter->save();
        painter->setTransform(DisplayApplication::viewSettings()->floorplanTransform(), true);
        painter->setPen(QPen(QBrush(Qt::black), 1));
        painter->drawPixmap(0, 0, pm);
        painter->drawRect(0, 0, pm.width(), pm.height());
        painter->restore();
    }
}

void GraphicsView::drawForeground(QPainter *painter, const QRectF &rect) {
    QGraphicsView::drawForeground(painter, rect);
    if(_tool) {
        _tool->draw(painter, rect, mapToScene(mapFromGlobal(QCursor::pos())));
    }
}

void GraphicsView::drawBackground(QPainter *painter, const QRectF &rect) {
    QGraphicsView::drawBackground(painter, rect);

    if(DisplayApplication::viewSettings()->getFloorplanShow()) {
        drawFloorplan(painter, rect);
    }
    if(DisplayApplication::viewSettings()->gridShow()) {
        drawGrid(painter, rect);
    }
    if(DisplayApplication::viewSettings()->originShow()) {
        drawOrigin(painter);
    }
}

void GraphicsView::setTool(AbstractTool *tool) {
    if(_tool) {
        QObject::disconnect(_tool, SIGNAL(done(bool)), this, SLOT(toolDone()));
        _tool->cancel();
        unsetCursor();
    }
    _tool = tool;
    if(_tool) {
        setCursor(_tool->cursor());
        QObject::connect(_tool, SIGNAL(done(bool)), this, SLOT(toolDone()));
        QObject::connect(_tool, SIGNAL(destroyed(QObject*)), this, SLOT(toolDestroyed()));
    }
}

void GraphicsView::floorplanChanged() {
    if(this->scene()) {
        this->scene()->update();
    }
}

void GraphicsView::toolDone() {
    if(QObject::sender() != _tool) return;
    QObject::disconnect(_tool, SIGNAL(done(bool)), this, SLOT(toolDone()));
    QObject::disconnect(_tool, SIGNAL(destroyed(QObject*)), this, SLOT(toolDestroyed()));
    unsetCursor();
    _tool = 0;

    if(this->scene()) {
        this->scene()->update();
    }
}

void GraphicsView::toolDestroyed() {
    unsetCursor();
    _tool = 0;
}

