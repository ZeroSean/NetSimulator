#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *const ui;
    QMenu *_helpMenu;
    QAction *_aboutAction;
    QLabel *_infoLabel;
};

#endif // MAINWINDOW_H
