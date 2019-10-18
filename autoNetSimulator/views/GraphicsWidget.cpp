#include "GraphicsWidget.h"
#include "ui_graphicswidget.h"

#include "DisplayApplication.h"
#include "ViewSettings.h"

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
    ui(new Ui::GraphicsWidget)
{

    QDesktopWidget desktop;
    int desktopHeight = desktop.geometry().height();
    int desktopWidth = desktop.geometry().width();

    ui->setupUi(this);
    this->_scene = new QGraphicsScene(this);

    ui->graphicsView->setScene(this->_scene);
    ui->graphicsView->scale(1, -1);
    ui->graphicsView->setBaseSize(desktopHeight * 0.75, desktopWidth * 0.75);

    _historyLength = 20;
    _showHistory = _showHistoryP = true;
    _busy = true;
    _ignore = true;
    _tagSize = 0.15;

    DisplayApplication::connectReady(this, "onReady()");
}

void GraphicsWidget::onReady() {
    QObject::connect(this, SIGNAL(centerAt(double,double)), graphicsView(), SLOT(centerAt(double, double)));
    QObject::connect(this, SIGNAL(centerRect(QRectF)), graphicsView(), SLOT(centerRect(QRectF)));

    _busy = false;
}

GraphicsWidget::~GraphicsWidget() {
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
    qDebug() << "table size: " << _tags.size();

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
            //删除历史数据
            for(int idx = 0; i < _historyLength; ++idx) {
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
    }
    _tags.clear();

    qDebug() << "clear tags";
}

void GraphicsWidget::tagIDToString(quint64 tagId, QString *t) {
    *t = "0x" + QString::number(tagId, 16);
}

void GraphicsWidget::insertTag(int id, QString &t, bool showR95, bool showLable, QString &lable) {

}

void GraphicsWidget::insertAnchor(int id, double x, double y, double z, int *array, bool show) {

}

void GraphicsWidget::addNewTag(quint64 tagId) {
    static double c_h = 0.1;
    Tag *tag = NULL;
    int tid = tagId;
    QString tagLabel = QString("Tag %1").arg(tid);

    qDebug() << "Add new Tag: 0x" + QString::number(tagId, 16) << tagId;

    _tags.insert(tagId, new(Tag));
    tag = this->_tags.value(tagId, NULL);
    if(tag) {
        tag->p.resize(_historyLength);

        c_h += 0.568034;
        if(c_h >= 1) {
            c_h -= 1;
        }
        tag->colourH = c_h;
        tag->colourS = 0.55;
        tag->colourV = 0.98;
        tag->tsPrev = 0;
        tag->r95Show = false;
        tag->showLabel = (tagLabel != NULL) ? true : false;
        tag->tagLabelStr = tagLabel;

        tag->r95p = this->_scene->addEllipse(0.1, 0.1, 0, 0);
        tag->r95p->setOpacity(0);
        tag->r95p->setPen(Qt::NoPen);
        tag->r95p->setBrush(Qt::NoBrush);

        tag->avgp = this->_scene->addEllipse(0, 0, 0, 0);
        tag->avgp->setOpacity(0);
        tag->avgp->setPen(Qt::NoPen);
        tag->avgp->setBrush(Qt::NoBrush);

        {
            QPen pen = QPen(Qt::blue);

            tag->tagLabel = new QGraphicsSimpleTextItem(NULL);
            tag->tagLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            tag->tagLabel->setZValue(1);
            tag->tagLabel->setText(tagLabel);
            tag->tagLabel->setOpacity(0);

            QFont font = tag->tagLabel->font();
            font.setPointSize(FONT_SIZE);
            font.setWeight(QFont::Normal);

            tag->tagLabel->setFont(font);

            pen.setStyle(Qt::SolidLine);
            pen.setWidthF(PEN_WIDTH);

            tag->tagLabel->setPen(pen);

            this->_scene->addItem(tag->tagLabel);
        }
    }
}

//更新tag在视图中的位置
void GraphicsWidget::tagPos(quint64 tagId, double x, double y, double z) {
    if(_busy) {
        qDebug() << "(Widget - busy IGNORE) Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;
    } else {
        Tag *tag = NULL;
        QString t;

        _busy = true;
        tagIDToString(tagId, &t);

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
                QBrush b = QBrush(QColor::fromHsvF(tag->colourH, tag->colourS, tag->colourV));
                QPen pen = QPen(b.color().darker());
                pen.setStyle(Qt::SolidLine);
                pen.setWidthF(PEN_WIDTH);

                tag_pt->setBrush(b.color().dark());
                tag->avgp->setBrush(b.color().darker());
                tag->tagLabel->setBrush(b.color().dark());
                tag->tagLabel->setPen(pen);
            }

            tag_pt->setToolTip(t);
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
    Tag &tag = this->_tags.value(tagId, NULL);
    for(int i = 0; i < _historyLength; ++i) {
        QAbstractGraphicsShapeItem *tag_p = tag.p[i];
        if(tag_p) {
            int j = tag.idx - i;
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
    for(Tag &tag : _tags) {
        tag.p.resize(value);
    }

    _historyLength = value;
    _showHistory = tag_showHistory;

    _busy = false;
}

void GraphicsWidget::CommunicateRangeValue(double value) {
    _commuRangeVal = value;
}

void GraphicsWidget::zone2Value(double value) {

}

//将所有基站居中显示
void GraphicsWidget::centerOnAnchor() {
    Anchor *a1 = this->_anchors.value(0, NULL);
    Anchor *a2 = this->_anchors.value(1, NULL);
    Anchor *a2 = this->_anchors.value(2, NULL);

    QPolygonF p1 = QPolygonF() << QPointF(a1->a->pos()) << QPointF(a2->a->pos()) << QPointF(a3->a->pos());

    emit centerRect(p1.boundingRect());
}

void GraphicsWidget::setShowTagHistory(bool show) {
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

    _anchors.insert(anchId, new(Anchor));
    anc = this->_anchors.value(anchId, NULL);
    anc->a = NULL;

    {
        anc->ancLabel = new QGraphicsSimpleTextItem(NULL);
        anc->ancLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        anc->ancLabel->setZValue(1);
        anc->ancLabel->setText(QString::number(ancId));

        QFont font = anc->ancLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);

        anc->ancLabel->setFocus(font);

        this->_scene->addItem(anc->ancLabel);
    }

    anc->show = show;
}

void GraphicsWidget::anchPos(quint64 anchId, double x, double y, double z, bool show) {
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
            QAbstractGraphicsShapeItem *anch = this->_scene->addEllipse(-ANC_SIZE / 2, -ANC_SIZE / 2, ANC_SIZE, ANC_SIZE);
            anch->setPen(Qt::NoPen);
            anch->setBrush(QBrush(QColor::fromRgb(85, 60, 150, 255)));
            anch->setToolTip("0x" + QString::number(anchId, 16));
            anc->a = anch;
        }

        anc->a->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setOpacity(anc->show ? 1.0 : 0.0);
        anc->a->setPos(x, y);
        anc->ancLabel->setPos(x + 0.15, y + 0.15);

        _busy = false;
    }
}



