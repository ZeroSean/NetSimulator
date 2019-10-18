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

    int applyFloorplanPic(const QString *path);

signals:
    void saveViewSetting(void);

protected slots:
    void onReady();

    void floorplanOpenClicked();
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

    void showGridOrigin(bool orig, bool grid);

    void getFloorplanPic(void);
    void showSave(void);

    void setTagHistory(int h);
    void loggingClicked(void);

private:
    Ui::ViewSettingsWidget *ui;

    bool _logging;
    bool _floorplanOpen;

};

#endif // VIEWSETTINGSWIDGET_H
