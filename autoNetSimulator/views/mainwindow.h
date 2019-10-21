#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
//#include <QAbstractSocket>

namespace Ui {
class MainWindow;
}

class GraphicsWidget;
class ViewSettingsWidget;

/**
 * 控制所有组件，全局菜单等
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    GraphicsWidget *graphicsWidget();

    ViewSettingsWidget *viewSettingsWidget();

    //使用窗口活动创建新的菜单，并返回菜单实例
    virtual QMenu *createPopupMenu();
    //添加窗口活动到菜单中
    QMenu *createPopupMenu(QMenu *menu);

    void saveConfigFile(QString filename, QString cfg);
    void loadConfigFile(QString filename);

public slots:
    void saveViewConfigSettings();

protected slots:
    void onReady();

    void loadSettings();
    void saveSettings();

    void onAboutAction();
    void onMiniMapView();

    void onAnchorConfigAction();
    void statusBarMessage(QString status);

private:
    Ui::MainWindow *const ui;
    QMenu *_helpMenu;
    QAction *_aboutAction;
    QLabel *_infoLabel;

    QAction *_anchorConfigAction;

    bool _showMainToolBar;
};

#endif // MAINWINDOW_H
