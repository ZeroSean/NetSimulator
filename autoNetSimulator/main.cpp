#include "DisplayApplication.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QSet<quint16> >("QSet<quint16>");

    DisplayApplication app(argc, argv);

    app.mainWindow()->show();

    return app.QApplication::exec();
}
