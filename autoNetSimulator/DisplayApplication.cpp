#include "DisplayApplication.h"

#include "mainwindow.h"

#include <QMetaProperty>
#include <QDesktopWidget>


DisplayApplication::DisplayApplication(int &argc, char **argv)
    : QApplication(argc, argv) {
    QDesktopWidget desktop;
    int desktopHeight = desktop.geometry().height();
    int desktopWidth = desktop.geometry().width();

    this->setOrganizationName("scut");
    this->setOrganizationDomain("scut.com");
    this->setApplicationName("Display");

    _mainWindow = new MainWindow();
    _mainWindow->resize(desktopWidth / 2, desktopHeight / 2);
}

DisplayApplication::~DisplayApplication() {
    //逆序析构对象
    delete _mainWindow;
}

DisplayApplication * DisplayApplication::instance() {
    return qobject_cast<DisplayApplication *>(QCoreApplication::instance());
}

MainWindow * DisplayApplication::mainWindow() {
    return instance()->_mainWindow;
}

