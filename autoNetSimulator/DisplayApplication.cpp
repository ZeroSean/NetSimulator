#include "DisplayApplication.h"

#include "mainwindow.h"
#include "ViewSettings.h"
#include "GraphicsWidget.h"
#include "ViewSettingsWidget.h"

#include <QMetaProperty>
#include <QDesktopWidget>


DisplayApplication::DisplayApplication(int &argc, char **argv)
    : QApplication(argc, argv) {
    QDesktopWidget desktop;
    int desktopHeight = desktop.geometry().height();
    int desktopWidth = desktop.geometry().width();

    _ready = false;

    this->setOrganizationName("scut");
    this->setOrganizationDomain("scut.com");
    this->setApplicationName("Display");

    _viewSettings = new ViewSettings(this);

    _mainWindow = new MainWindow();
    _mainWindow->resize(desktopWidth / 2, desktopHeight / 2);

    _ready = true;

    emit ready();
}

DisplayApplication::~DisplayApplication() {
    //逆序析构对象
    delete _mainWindow;

    delete _viewSettings;
}

DisplayApplication * DisplayApplication::instance() {
    return qobject_cast<DisplayApplication *>(QCoreApplication::instance());
}

MainWindow * DisplayApplication::mainWindow() {
    return instance()->_mainWindow;
}

ViewSettings *DisplayApplication::viewSettings() {
    return instance()->_viewSettings;
}

GraphicsWidget *DisplayApplication::graphicsWidget() {
    return mainWindow()->graphicsWidget();
}

ViewSettingsWidget *DisplayApplication::viewSettingsWidget() {
    return mainWindow()->viewSettingsWidget();
}

GraphicsView *DisplayApplication::graphicsView() {
    return mainWindow()->graphicsWidget()->graphicsView();
}

void DisplayApplication::connectReady(QObject *receiver, const char *member, Qt::ConnectionType type) {
    QMetaMethod method = receiver->metaObject()->method(receiver->metaObject()->indexOfMethod(QMetaObject::normalizedSignature(member)));

    if(instance()->_ready && method.isValid()) {
        method.invoke(receiver, type);
    } else {
        QObject::connect(instance(), QMetaMethod::fromSignal(&DisplayApplication::ready), receiver, method, type);
    }
}




