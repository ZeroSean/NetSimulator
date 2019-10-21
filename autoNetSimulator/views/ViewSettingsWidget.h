#ifndef VIEWSETTINGSWIDGET_H
#define VIEWSETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class ViewSettingsWidget;
}


class ViewSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ViewSettingsWidget(QWidget *parent = 0);
    ~ViewSettingsWidget();

    int applyFloorplanPic(const QString &path);

signals:
    void saveViewSettings(void);

protected slots:
    void onReady();

    void floorplanOpenClicked();
    void ancConfigSelectClicked();
    void originClicked();
    void scaleClicked();
    void gridShowClicked();
    void originShowClicked();
    void tagHistoryShowClicked();

    void saveFPClicked();

    void communicateRangeChanged(double value);
    void communicateRangeEditFinished(void);
    void zone2ValueChanged(double value);
    void zone2EditFinished(void);
    void tagHistoryNumberValueChanged(int value);

    void showGridOrigin(bool grid, bool orig);

    void getFloorplanPic(void);
    void showSave(bool arg);

    void setTagHistory(int h);
    void loggingClicked(void);

private:
    Ui::ViewSettingsWidget *ui;

    bool _logging;
    bool _floorplanOpen;
    bool _ancConfigSelect;

};

#endif // VIEWSETTINGSWIDGET_H
