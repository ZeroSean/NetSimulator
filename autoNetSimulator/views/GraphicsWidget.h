#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H

#include <QGraphicsRectItem>
#include <QWidget>
#include <QAbstractItemView>
#include <QGraphicsView>

#include "Coordinator.h"
#include "InstanceAnch.h"

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
        idx = 0;
    }
    quint64 id;
    int idx;    //history index
    QVector<QAbstractGraphicsShapeItem *> p;
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
    }
    quint64 id;
    QAbstractGraphicsShapeItem *a;
    QAbstractGraphicsShapeItem *range;
    QGraphicsSimpleTextItem *ancLabel;
    bool show;
    QPointF pos;

    double x;
    double y;
    double z;
    double commuRange;
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

signals:
    void updateAnchorXYZ(int id, int x, double value);
    void updateTagCorrection(int aid, int tid, int value);

    void centerAt(double x, double y);
    void centerRect(const QRectF &visibleRect);

    void setTagHistory(int h);

public slots:
    void centerOnAnchor(void);

    void tagPos(quint64 tagId, double x, double y, double z);
    void tagStats(quint64 tagId, double x, double y, double z, double r95);
    void tagRange(quint64 tagId, quint64 aId, double range);
    void anchPos(quint64 anchId, double x, double y, double z, double comRange, bool show);
    void ancCommunicateRange(Anchor * anc, double x, double y, double z, double comRange, bool show);

    void clearTags(void);
    void clearAnchors(void);

    void setShowTagHistory(bool show);
    void communicateRangeValue(double value);
    void zone2Value(double value);
    void tagHistoryNumber(int value);

    void ancConfigFileChanged();

protected slots:
    void onReady();

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

};

#endif // GRAPHICSWIDGET_H
