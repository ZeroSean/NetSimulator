#include "GraphicsWidget.h"
#include "ui_graphicswidget.h"

#include "DisplayApplication.h"
#include "ViewSettings.h"
#include "ViewSettingsWidget.h"

#include <QDomDocument>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QInputDialog>
#include <QFile>
#include <QPen>
#include <QDesktopWidget>

#define PEN_WIDTH   (0.04)
#define ANC_SIZE    (0.15)
#define FONT_SIZE   (10)


GraphicsWidget::GraphicsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphicsWidget),
    _coor(NULL)
{
    QDesktopWidget desktop;
    int desktopHeight = desktop.geometry().height();
    int desktopWidth = desktop.geometry().width();

    ui->setupUi(this);
    this->_scene = new QGraphicsScene(this);

    ui->graphicsView->setScene(this->_scene);
    ui->graphicsView->scale(1, -1);
    ui->graphicsView->setBaseSize(desktopHeight, desktopWidth);

    _historyLength = 20;
    _showHistory = _showHistoryP = false;
    _busy = true;
    _ignore = true;
    _tagSize = 0.15;

    insTag = NULL;

    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(ancConfigFileChanged()), this, SLOT(ancConfigFileChanged()));

    DisplayApplication::connectReady(this, "onReady()");
}

void GraphicsWidget::onReady() {
    QObject::connect(this, SIGNAL(centerAt(double,double)), graphicsView(), SLOT(centerAt(double, double)));
    QObject::connect(this, SIGNAL(centerRect(QRectF)), graphicsView(), SLOT(centerRect(QRectF)));

    _busy = false;
}

GraphicsWidget::~GraphicsWidget() {
    if(_coor) {
        _coor->requestInterruption();
        //QThread::usleep(2000);
        delete _coor;
        _coor = NULL;
        qDebug() << "coordinator thread exit!";
    }

    clearAnchors();
    clearTags();

    delete _scene;
    delete ui;
}

GraphicsView *GraphicsWidget::graphicsView() {
    return ui->graphicsView;
}

void GraphicsWidget::loadConfigFile(QString filename) {
    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly)) {
        qDebug(qPrintable(QString("Error: cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }

    QDomDocument doc;
    QString error;
    int errorLine;
    int errorColumn;

    if(doc.setContent(&file, false, &error, &errorLine, &errorColumn)) {
        qDebug() << "file error !!!" << error << errorLine << errorColumn;
        emit setTagHistory(_historyLength);
    }

    QDomElement config = doc.documentElement();

    if(config.tagName() == "config") {
        QDomNode n = config.firstChild();
        while(!n.isNull()) {
            QDomElement e = n.toElement();
            if(!e.isNull()) {
                if(e.tagName() == "tag_cfg") {
                    _tagSize = (e.attribute("size", "")).toDouble();
                    _historyLength = (e.attribute("history", "")).toInt();

                } else if(e.tagName() == "tag"){
                    bool ok;
                    quint64 id = (e.attribute("ID", "")).toLongLong(&ok, 16);
                    QString label = (e.attribute("label", ""));

                    _tagLabels.insert(id, label);
                }
            }
            n = n.nextSibling();
        }
    }

    file.close();

    emit setTagHistory(_historyLength);
}

void GraphicsWidget::saveConfigFile(QString filename) {
    QFile file(filename);

    if(!file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./tag_config").arg(file.errorString())));
        return;
    }

    QDomDocument doc;

    QDomElement config = doc.createElement("config");
    doc.appendChild(config);

    QDomElement cn = doc.createElement("tag_cfg");
    cn.setAttribute("size", QString::number(_tagSize));
    cn.setAttribute("history", QString::number(_historyLength));
    config.appendChild(cn);

    QMap<quint64, QString>::iterator i = _tagLabels.begin();
    while(i != _tagLabels.end()) {
        QDomElement cn = doc.createElement("tag");
        cn.setAttribute("ID", QString::number(i.key(), 16));
        cn.setAttribute("label", i.value());
        config.appendChild(cn);

        i++;
    }

    QTextStream ts(&file);
    ts << doc.toString();

    file.close();

    qDebug() << doc.toString();
}

void GraphicsWidget::clearTags() {
    qDebug() << "tags size: " << _tags.size();

    QMap<quint64, Tag*>::iterator i = _tags.begin();
    while(i != _tags.end()) {
        quint64 tagID = i.key();
        Tag *tag = i.value();

        qDebug() << "Item Tag ID: " << tagID;

        if(tag) {
            if(tag->r95p) {
                tag->r95p->setOpacity(0);   //隐藏
                this->_scene->removeItem(tag->r95p);
                delete(tag->r95p);
                tag->r95p = NULL;
            }
            if(tag->avgp) {
                tag->avgp->setOpacity(0);
                this->_scene->removeItem(tag->avgp);
                delete(tag->avgp);
                tag->avgp = NULL;
            }
            if(tag->tagLabel) {
                tag->tagLabel->setOpacity(0);
                this->_scene->removeItem(tag->tagLabel);
                delete(tag->tagLabel);
                tag->tagLabel = NULL;
            }
            if(tag->routeLine) {
                tag->routeLine->setOpacity(0);
                this->_scene->removeItem(tag->routeLine);
                delete(tag->routeLine);
                tag->routeLine = NULL;
            }

            //删除历史数据
            for(int idx = 0; idx < _historyLength; ++idx) {
                QAbstractGraphicsShapeItem *tag_p = tag->p[idx];
                if(tag_p) {
                    tag_p->setOpacity(0);
                    this->_scene->removeItem(tag_p);
                    delete(tag_p);
                    tag->p[idx] = NULL;

                    qDebug() << "history remove tag" << idx;
                }
            }
        }
        delete(tag);
        *i = NULL;
        i++;
    }
    _tags.clear();

    if(insTag) {
        QObject::disconnect(insTag, SIGNAL(tagConnectFinished(quint16,quint16,bool)), this, SLOT(drawRoutePathFromTag(quint16,quint16,bool)));
        delete insTag;
    }

    qDebug() << "clear tags";
}

void GraphicsWidget::clearRouteLines() {
    for(Tag *tag : _tags) {
        if(tag->routeLine) {
            tag->routeLine->setOpacity(0);
            this->_scene->removeItem(tag->routeLine);
            delete(tag->routeLine);
            tag->routeLine = NULL;
        }
    }

    for(QGraphicsLineItem *line : _routePath) {
        QPen pen = QPen(QBrush(Qt::red), 0.005);
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(0.03);
        line->setPen(pen);
    }
    _routePath.clear();
}

void GraphicsWidget::clearAnchors() {
    drawRoutePath(0, 0, false);     //clear all routh path lines

    QMap<quint64, Anchor*>::iterator i = _anchors.begin();
    while(i != _anchors.end()) {
        //quint64 ancId = i.key();
        Anchor *anc = i.value();

        if(anc) {
            if(anc->a) {
                anc->a->setOpacity(0);   //隐藏
                this->_scene->removeItem(anc->a);
                delete(anc->a);
                anc->a = NULL;
            }
            if(anc->ancLabel) {
                anc->ancLabel->setOpacity(0);   //隐藏
                this->_scene->removeItem(anc->ancLabel);
                delete(anc->ancLabel);
                anc->ancLabel = NULL;
            }
            if(anc->range) {
                anc->range->setOpacity(0);   //隐藏
                this->_scene->removeItem(anc->range);
                delete(anc->range);
                anc->range = NULL;
            }

            if(!anc->connectLines.isEmpty()) {
                for(QGraphicsLineItem *item : anc->connectLines.values()) {
                    item->setOpacity(0);
                    this->_scene->removeItem(item);
                    delete item;
                }
                anc->connectLines.clear();
            }

            anc->allLines.clear();
        }
        delete(anc);
        *i = NULL;
        i++;
    }
    _anchors.clear();

    for(InstanceAnch *ins : _insAnchors) {
        QObject::disconnect(ins, SIGNAL(netConnectFinished(quint16,QSet<quint16>,quint16)), this, SLOT(netConnectFinished(quint16,QSet<quint16>,quint16)));
        delete ins;
    }
    _insAnchors.clear();

    qDebug() << "clear anchors";
}

void GraphicsWidget::tagIDToString(quint64 tagId, QString *t) {
    *t = "0x" + QString::number(tagId, 16);
}

void GraphicsWidget::insertTag(int id, QString &t, bool showR95, bool showLable, QString &lable) {
    Q_UNUSED(id)
    Q_UNUSED(t)
    Q_UNUSED(showR95)
    Q_UNUSED(showLable)
    Q_UNUSED(lable)
}

void GraphicsWidget::insertAnchor(int id, double x, double y, double z, int *array, bool show) {
    Q_UNUSED(id)
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(z)
    Q_UNUSED(array)
    Q_UNUSED(show)
}

void GraphicsWidget::addNewTag(quint64 tagId) {
    Tag *tag = NULL;
    int tid = tagId;
    QString tagLabel = QString("Tag %1").arg(tid);

    qDebug() << "Add new Tag: 0x" + QString::number(tagId, 16) << tagId;

    _tags.insert(tagId, new(Tag));
    tag = this->_tags.value(tagId, NULL);
    if(tag) {
        tag->id = tagId;
        tag->p.resize(_historyLength);

        tag->showLabel = (tagLabel != NULL) ? true : false;
        tag->tagLabelStr = tagLabel;
        tag->tagLabel = new QGraphicsSimpleTextItem(NULL);
        tag->tagLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        tag->tagLabel->setZValue(1);
        tag->tagLabel->setText(tagLabel);
        tag->tagLabel->setOpacity(0);


        QPen pen = QPen(Qt::blue);
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(PEN_WIDTH);
        tag->tagLabel->setPen(pen);

        QFont font = tag->tagLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);
        tag->tagLabel->setFont(font);

        this->_scene->addItem(tag->tagLabel);
    }
}

//更新tag在视图中的位置
void GraphicsWidget::tagPos(quint64 tagId, double x, double y, double z) {
    if(_busy) {
        qDebug() << "(Widget - busy IGNORE) Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;
    } else {
        Tag *tag = NULL;

        _busy = true;

        tag = _tags.value(tagId, NULL);
        if(!tag) {
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }

        if(tag && (!tag->p[tag->idx])) {
            QAbstractGraphicsShapeItem *tag_pt = this->_scene->addEllipse(-1 * _tagSize / 2, -1 * _tagSize / 2,
                                                                          _tagSize, _tagSize);
            tag->p[tag->idx] = tag_pt;
            tag_pt->setPen(Qt::NoPen);

            if(tag->idx > 0) {
                tag_pt->setBrush(tag->p[0]->brush());
            } else {
                QBrush b = QBrush(Qt::black);
                QPen pen = QPen(b.color().darker());
                pen.setStyle(Qt::SolidLine);
                pen.setWidthF(PEN_WIDTH);

                tag_pt->setBrush(b.color().dark());
                tag->tagLabel->setBrush(b.color().dark());
                tag->tagLabel->setPen(pen);
            }

            tag_pt->setToolTip("0x" + QString::number(tagId, 16));
        }

        _ignore = true;
        tag->p[tag->idx]->setPos(x, y);
        if(_showHistory) {
            tagHistory(tagId);
            tag->idx = (tag->idx + 1) % _historyLength;
        } else {
            tag->p[tag->idx]->setOpacity(1);
        }

        tag->tagLabel->setPos(x + 0.15, y + 0.15);

        tag->x = x;
        tag->y = y;
        tag->z = z;
        tag->commuRange = _commuRangeVal;

        _ignore = false;
        _busy = false;
    }
}

void GraphicsWidget::tagStats(quint64 tagId, double x, double y, double z, double r95) {
    if(_busy) {
        qDebug() << "(busy IGNORE) R95: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z << " " << r95;
        return;
    } else {
        Tag *tag = NULL;

        _busy = true;
        tag = _tags.value(tagId, NULL);

        if(!tag) {
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }

        if(tag) {
            double rad = r95 * 2;

            if(tag->r95p) {
                tag->r95p->setOpacity(0);
                this->_scene->removeItem(tag->r95p);
                delete(tag->r95p);
                tag->r95p = NULL;
            }
            {
                tag->r95p = this->_scene->addEllipse(-1 * r95, -1 * r95, rad, rad);
                tag->r95p->setPen(Qt::NoPen);
                tag->r95p->setBrush(Qt::NoBrush);

                if(tag->r95Show && (rad <= 1)) {
                    QPen pen = QPen(Qt::darkGreen);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    tag->r95p->setOpacity(0.5);
                    tag->r95p->setPen(pen);
                    tag->r95p->setBrush(Qt::NoBrush);
                } else if(tag->r95Show && (rad > 1)){
                    QPen pen = QPen(Qt::darkRed);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    tag->r95p->setOpacity(0.5);
                    tag->r95p->setPen(pen);
                    tag->r95p->setBrush(Qt::NoBrush);
                }
            }

            if(!tag->avgp) {
                QBrush b = tag->p[0]->brush().color().darker();

                tag->avgp = this->_scene->addEllipse(-0.025, -0.025, 0.05, 0.05);
                tag->avgp->setBrush(b);
                tag->avgp->setPen(Qt::NoPen);
            }

            if(tag->r95Show) {
                tag->avgp->setOpacity(1);
                tag->avgp->setPos(x, y);
            } else {
                tag->avgp->setOpacity(0);
            }
            tag->r95p->setPos(x, y);
        }

        _busy = false;

        qDebug() << "R95: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z << " " << r95;
    }
}

void GraphicsWidget::tagRange(quint64 tagId, quint64 aId, double range) {
    if(_busy) {
        qDebug() << "(busy IGNORE) Range 0x" + QString::number(tagId, 16) << " " << range << " " << QString::number(aId, 16);

    } else {
        Tag *tag = _tags.value(tagId, NULL);

        _busy = true;

        if(!tag) {
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }
        if(tag) {

        }

        _busy = false;
    }
}

void GraphicsWidget::tagHistory(quint64 tagId) {
    Tag *tag = this->_tags.value(tagId, NULL);
    for(int i = 0; i < _historyLength; ++i) {
        QAbstractGraphicsShapeItem *tag_p = tag->p[i];
        if(tag_p) {
            int j = tag->idx - i;
            if(j < 0) {
                j += _historyLength;
            }

            tag_p->setOpacity(1 - (float)j / _historyLength);
        }
    }
}

void GraphicsWidget::tagHistoryNumber(int value) {
    bool tag_showHistory = _showHistory;

    while(_busy);
    _busy = true;

    //先移除旧的历史数据
    setShowTagHistory(false);
    for(Tag *tag : _tags) {
        tag->p.resize(value);
    }

    _historyLength = value;
    _showHistory = tag_showHistory;

    _busy = false;
}

void GraphicsWidget::communicateRangeValue(double value) {
    if(value > 0 && fabs(_commuRangeVal - value) > 0.001) {
        _commuRangeVal = value;

        for(Anchor *anc : _anchors) {
            ancCommunicateRange(anc, anc->x, anc->y, anc->z, _commuRangeVal, true);
        }
    }
}

void GraphicsWidget::zone2Value(double value) {
    Q_UNUSED(value)
}

//将所有基站居中显示
void GraphicsWidget::centerOnAnchor() {
    Anchor *a1 = this->_anchors.value(0, NULL);
    Anchor *a2 = this->_anchors.value(1, NULL);
    Anchor *a3 = this->_anchors.value(2, NULL);

    QPolygonF p1 = QPolygonF() << QPointF(a1->a->pos()) << QPointF(a2->a->pos()) << QPointF(a3->a->pos());

    emit centerRect(p1.boundingRect());
}

void GraphicsWidget::setShowTagHistory(bool set) {
    _busy = true;

    if(set != _showHistory) {
        if(set == false) {
            QMap<quint64, Tag*>::iterator i = _tags.begin();

            while(i != _tags.end()) {
                Tag *tag = i.value();

                for(int idx = 0; idx < _historyLength; ++idx) {
                    QAbstractGraphicsShapeItem *tag_p = tag->p[idx];
                    if(tag_p) {
                        tag_p->setOpacity(0);

                        this->_scene->removeItem(tag_p);
                        delete(tag_p);
                        tag_p = NULL;
                        tag->p[idx] = 0;
                    }
                }
                tag->idx = 0;
                ++i;
            }
        }

        _showHistory = _showHistoryP = set;
    }

    _busy = false;
}

void GraphicsWidget::addNewAnchor(quint64 ancId, bool show) {
    Anchor *anc;

    qDebug() << "Add new Anchor: 0x:" << QString::number(ancId, 16);

    _anchors.insert(ancId, new(Anchor));
    anc = this->_anchors.value(ancId, NULL);
    anc->a = NULL;
    anc->range = NULL;
    anc->id = ancId;

    {
        anc->ancLabel = new QGraphicsSimpleTextItem(NULL);
        anc->ancLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        anc->ancLabel->setZValue(1);
        anc->ancLabel->setText(QString::number(ancId));

        QFont font = anc->ancLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);

        anc->ancLabel->setFont(font);

        this->_scene->addItem(anc->ancLabel);
    }

    anc->show = show;
}

void GraphicsWidget::ancCommunicateRange(Anchor * anc, double x, double y, double z, double comRange, bool show) {
    Q_UNUSED(show)
    Q_UNUSED(z)

    if(anc->range != NULL) {
        this->_scene->removeItem(anc->range);
        delete (anc->range);
        anc->range = NULL;
    }

    QAbstractGraphicsShapeItem *anch = this->_scene->addEllipse(-comRange, -comRange, 2 * comRange, 2 * comRange);
    anch->setPen(Qt::NoPen);
    anch->setBrush(QBrush(QColor::fromRgb(102, 171, 13, 128))); //浅绿色
    anch->setToolTip("0x" + QString::number(anc->id, 16));
    anc->range = anch;

    QPen pen = QPen(QBrush(Qt::green), 0.005) ;
    //pen.setColor(Qt::red);
    pen.setStyle(Qt::SolidLine);
    pen.setWidthF(0.03);

    anch->setPen(pen);

    anc->range->setOpacity(anc->show ? 1.0 : 0.0);
    anc->range->setPos(x, y);

    anc->commuRange = comRange;
}

void GraphicsWidget::anchPos(quint64 anchId, double x, double y, double z, double comRange, bool show, uint8_t gateway) {
    if(_busy) {
        qDebug() << "(Widget - busy IGNORE) anch: 0x" + QString::number(anchId, 16) << " " << x << " " << y << " " << z;
    } else {
        Anchor *anc = _anchors.value(anchId, NULL);

        _busy = true;

        if(!anc) {
            addNewAnchor(anchId, show);
            anc = this->_anchors.value(anchId, NULL);
        }

        if(anc->a == NULL) {
            QAbstractGraphicsShapeItem *anch = NULL;

            if(gateway) {
                anch = this->_scene->addEllipse(-ANC_SIZE, -ANC_SIZE, 2 * ANC_SIZE, 2 * ANC_SIZE);;
                anch->setBrush(QBrush(QColor::fromRgb(255, 0, 0, 255)));
            } else {
                anch = this->_scene->addEllipse(-ANC_SIZE / 2, -ANC_SIZE / 2, ANC_SIZE, ANC_SIZE);;
                anch->setBrush(QBrush(QColor::fromRgb(85, 60, 150, 255)));
            }
            anch->setZValue(5);    //绘制在最顶部

            anch->setPen(Qt::NoPen);
            anch->setToolTip("0x" + QString::number(anchId, 16));
            anc->a = anch;
        }

        anc->a->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setOpacity(anc->show ? 1.0 : 0.0);

        anc->a->setPos(x, y);
        anc->ancLabel->setPos(x + 0.15, y + 0.15);

        anc->x = x;
        anc->y = y;
        anc->z = z;
        anc->gateway = gateway;

        ancCommunicateRange(anc, x, y, z, comRange, show);

        _busy = false;
    }
}

void GraphicsWidget::ancConfigFileChanged() {
    QString path = DisplayApplication::viewSettings()->getAncConfigFilePath();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug(qPrintable(QString("Error: cannot read file (%1) %2").arg(path).arg(file.errorString())));
        return;
    } else {
        QTextStream stream(&file);
        QString line = "";

        if(_coor != NULL) {
            _coor->requestInterruption();
            QThread::usleep(2000);
            delete _coor;
            _coor = NULL;
        }
        _coor = new Coordinator();

        clearAnchors();

        if(_commuRangeVal < 0.2) {
            _commuRangeVal = 0.2;
        }


        while(!(line = stream.readLine()).isNull()) {
            qDebug() << line;

            std::string cLine = line.toStdString();
            int id;
            double x, y, z;
            int gateway = 0;
            sscanf(cLine.c_str(), "%d:(%lf, %lf, %lf):%d", &id, &x, &y, &z, &gateway);

            anchPos(id, x, y, z, _commuRangeVal, true, gateway);
            qDebug() << id << x << y << z << gateway;

            InstanceAnch *ins = new InstanceAnch(_coor, gateway);
            _coor->addAnchor(ins, id, x, y, z, _commuRangeVal, 1);

            _insAnchors.insert(id, ins);

            QObject::connect(ins, SIGNAL(netConnectFinished(quint16,QSet<quint16>,quint16)),
                             this, SLOT(netConnectFinished(quint16,QSet<quint16>,quint16)));
        }

        _coor->start();
    }
}

QGraphicsLineItem * GraphicsWidget::addNewLine(Anchor *anc1, Anchor *anc2) {

    if(anc1 != NULL && anc2 != NULL) {
        QGraphicsLineItem *line = this->_scene->addLine(anc1->x, anc1->y, anc2->x, anc2->y);
        QPen pen = QPen(QBrush(Qt::red), 0.005);
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(0.03);

        line->setPen(pen);
        line->setOpacity(1);

        line->setZValue(10);    //绘制在最顶部

        anc1->connectLines.insert(anc2->id, line);
        anc1->allLines.insert(anc2->id, line);
        anc2->allLines.insert(anc1->id, line);

        return line;
    }
    return NULL;
}

void GraphicsWidget::netConnectFinished(quint16 src, QSet<quint16> dsts, quint16 seat) {
    while(_busy);
    _busy = true;

    Anchor *anc = _anchors.value(src, NULL);

    if(!anc) {
        qDebug() << "unfound anch: 0x" + QString::number(src, 16) << " seat:" << seat;
        return;
    }

    if(!anc->connectLines.isEmpty()) {
        for(QGraphicsLineItem *item : anc->connectLines.values()) {
            item->setOpacity(0);
            this->_scene->removeItem(item);
            delete item;
        }
        anc->connectLines.clear();
    }

    for(uint16 id : dsts) {
        Anchor *dstAnc = _anchors.value(id, NULL);

        if(dstAnc) {
            addNewLine(anc, dstAnc);
        }
    }

    anc->a->setToolTip("Seat:" + QString::number(seat));

    _busy = false;
}

void GraphicsWidget::drawRoutePath(uint16_t start, uint16_t end, bool show) {
    clearRouteLines();

    if(_coor) {
        _coor->runTag(false);
    }

    if(show) {
        uint16_t pre = start;
        int error = 0;
        QString info = "out--len--dest\n";

        while(pre != end) {
            InstanceAnch *anch = _insAnchors.value(pre, NULL);
            Anchor *startAnc = _anchors.value(pre, NULL);

            if(anch == NULL || startAnc == NULL) {
                error = 1;
                emit routeMsgShow("cant find anchor:" + QString::number(pre));
                break;
            }

            int next = anch->getRouteOutPort(end);
            if(next == -1) {
                emit routeMsgShow("cant find out port in route table:" + QString::number(pre));
                error = 2;
                break;
            }

            QGraphicsLineItem *line = startAnc->allLines.value(next, NULL);
            if(line == NULL) {
                line = addNewLine(startAnc, _anchors.value(next, NULL));

                if(line == NULL) {
                    emit routeMsgShow("cant find line:" + QString::number(pre) + "--->" + QString::number(next));
                    error = 3;
                    break;
                }
            }

            info += "Anchor: " + QString::number(pre) + "\n";
            info += anch->routeToString() + "\n";

            _routePath.insert(line);
            pre = next;
        }

        if(error == 0) {
            QPen pen = QPen(QBrush(Qt::darkRed), 0.005);
            pen.setStyle(Qt::DotLine);
            pen.setWidthF(0.05);

            for(QGraphicsLineItem *line : _routePath) {
                line->setPen(pen);
            }

            emit routeMsgShow(info);
        } else {
            _routePath.clear();
        }
    }
}


//onlu one tag of id[0x8001] be used to draw route path
void GraphicsWidget::tagConfigChanged(double x, double y) {
    tagPos(0x8001, x, y, 0);

    if(_coor && insTag == NULL) {
        insTag = new InstanceTag(_coor);
        _coor->addTag(insTag, 0x8001, x, y, 0, _commuRangeVal, 1);

        QObject::connect(insTag, SIGNAL(tagConnectFinished(quint16,quint16,bool)), this, SLOT(drawRoutePathFromTag(quint16,quint16,bool)));
    }

    if(insTag){
        insTag->setPos(x, y, 0);
        insTag->clearBCNLog();
    }

    if(_coor) {
        _coor->runTag(true);
    }

    //drawRoutePathFromTag(0x8001, 15, true);
}

void GraphicsWidget::drawRoutePathFromTag(quint16 tagId, quint16 start, bool show) {
    Tag *tag = _tags.value(tagId, NULL);
    int error = 0;
    QString info = "out--len--dest\n";

    clearRouteLines();

    if(show && tag != NULL) {
        Anchor *anc = _anchors.value(start, NULL);

        if(anc == NULL) {
            error = 5;
            emit routeMsgShow("cant find anchor:" + QString::number(start));
        } else {
            if(tag->routeLine == NULL) {
                tag->routeLine = this->_scene->addLine(tag->x, tag->y, anc->x, anc->y);
            }
            QPen pen = QPen(QBrush(Qt::darkRed), 0.005);
            pen.setStyle(Qt::DotLine);
            pen.setWidthF(0.05);

            tag->routeLine->setPen(pen);
            tag->routeLine->setOpacity(1);
            tag->routeLine->setZValue(10);    //绘制在最顶部
        }
    } else {
        error = 6;
    }

    if(show) {
        uint16_t pre = start;

        while(true) {
            InstanceAnch *insAnc = _insAnchors.value(pre, NULL);
            Anchor *beginAnc = _anchors.value(pre, NULL);

            if(insAnc == NULL || beginAnc == NULL) {
                error = 1;
                emit routeMsgShow("cant find anchor:" + QString::number(pre));
                break;
            } else if(insAnc->isGateway()) {
                break;
            }

            int next = insAnc->getGatewayOutPort();
            if(next == -1) {
                emit routeMsgShow("cant find gateway out port in route table:" + QString::number(pre));
                error = 2;
                break;
            }

            QGraphicsLineItem *line = beginAnc->allLines.value(next, NULL);
            if(line == NULL) {
                line = addNewLine(beginAnc, _anchors.value(next, NULL));

                if(line == NULL) {
                    emit routeMsgShow("cant find line:" + QString::number(pre) + "--->" + QString::number(next));
                    error = 3;
                    break;
                }
            }

            info += "Anchor: " + QString::number(pre) + "\n";
            info += insAnc->routeToString() + "\n";

            _routePath.insert(line);
            pre = next;
        }


    }

    if(error == 0) {
        QPen pen = QPen(QBrush(Qt::darkRed), 0.005);
        pen.setStyle(Qt::DotLine);
        pen.setWidthF(0.05);

        for(QGraphicsLineItem *line : _routePath) {
            line->setPen(pen);
        }

        emit routeMsgShow(info);
    } else {
        clearRouteLines();
    }
}

