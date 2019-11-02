#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H

#include <QGraphicsRectItem>
#include <QWidget>
#include <QAbstractItemView>
#include <QGraphicsView>

#include "Coordinator.h"
#include "InstanceAnch.h"
#include "InstanceTag.h"

namespace Ui{
class GraphicsWidget;
}

class GraphicsDataModel;    //none
class QGraphicsScene;
class QModelIndex;
class GraphicsView;
class QAbstractGraphicsShapeItem;
class QGraphicsItem;

struct Tag{
    Tag(void) {
        id = 0;
        idx = 0;
        routeLine= NULL;
        avgp = NULL;
        r95p = NULL;
        tagLabel= NULL;
    }

    quint64 id;
    int idx;    //history index
    QVector<QAbstractGraphicsShapeItem *> p;
    QGraphicsLineItem *routeLine;

    QAbstractGraphicsShapeItem *avgp;
    QAbstractGraphicsShapeItem *r95p;   //r95 circle around the average point, average of 100

    QGraphicsSimpleTextItem *tagLabel;
    QString tagLabelStr;

    bool r95Show;
    bool showLabel;

    double tsPrev;          //previous timestamp in sec
    double colourH;
    double colourS;
    double colourV;

    double x;
    double y;
    double z;
    double commuRange;
};

struct Anchor{
    Anchor(void) {
        id = 0;
        a = NULL;
        range = NULL;
        ancLabel = NULL;
        x = 0;
        y = 0;
        z = 0;
        commuRange = 5;
        gateway = 0;
    }
    quint64 id;
    QAbstractGraphicsShapeItem *a;
    QAbstractGraphicsShapeItem *range;
    QGraphicsSimpleTextItem *ancLabel;

    QMap<quint64, QGraphicsLineItem *> connectLines;
    QMap<quint64, QGraphicsLineItem *> allLines;

    bool show;
    QPointF pos;

    double x;
    double y;
    double z;
    double commuRange;
    uint8_t gateway;
};

class GraphicsWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphicsWidget(QWidget *parent = 0);
    ~GraphicsWidget();

    GraphicsView *graphicsView();
    void loadConfigFile(QString filename);
    void saveConfigFile(QString filename);

    void addNewAnchor(quint64 ancId, bool show);
    void insertAnchor(int id, double x, double y, double z, int *array, bool show);
    void addNewTag(quint64 tagId);
    void insertTag(int id, QString &t, bool showR95, bool showLable, QString &lable);
    void tagIDToString(quint64 tagId, QString *t);

    QGraphicsLineItem * addNewLine(Anchor *anc1, Anchor *anc2);

signals:
    void updateAnchorXYZ(int id, int x, double value);
    void updateTagCorrection(int aid, int tid, int value);

    void centerAt(double x, double y);
    void centerRect(const QRectF &visibleRect);

    void setTagHistory(int h);

    void routeMsgShow(QString msg);

public slots:
    void centerOnAnchor(void);

    void tagPos(quint64 tagId, double x, double y, double z);
    void tagStats(quint64 tagId, double x, double y, double z, double r95);
    void tagRange(quint64 tagId, quint64 aId, double range);
    void anchPos(quint64 anchId, double x, double y, double z, double comRange, bool show, uint8_t gateway = 0);
    void ancCommunicateRange(Anchor * anc, double x, double y, double z, double comRange, bool show);

    void clearTags(void);
    void clearAnchors(void);
    void clearRouteLines(void);

    void setShowTagHistory(bool show);
    void communicateRangeValue(double value);
    void zone2Value(double value);
    void tagHistoryNumber(int value);

    void ancConfigFileChanged();

    void tagConfigChanged(double x, double y);

    void drawRoutePath(uint16_t start, uint16_t end, bool show);


protected slots:
    void onReady();

    void drawRoutePathFromTag(quint16 tagId, quint16 start, bool show);
    void netConnectFinished(quint16 src, QSet<quint16> dst, quint16 seat);

protected:
    void tagHistory(quint64 tagId);

private:
    Ui::GraphicsWidget *ui;
    QGraphicsScene *_scene;

    QMap<quint64, Tag*> _tags;
    QMap<quint64, Anchor*> _anchors;
    QMap<quint64, QString> _tagLabels;

    float _tagSize;
    int _historyLength;
    int _showHistory;
    int _showHistoryP;
    bool _busy;
    bool _ignore;

    double _commuRangeVal;

    Coordinator *_coor;
    QMap<quint64, InstanceAnch*> _insAnchors;

    InstanceTag *insTag;


    QSet<QGraphicsLineItem *> _routePath;
};

#endif // GRAPHICSWIDGET_H
