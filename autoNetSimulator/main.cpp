#include "DisplayApplication.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    DisplayApplication app(argc, argv);

    app.mainWindow()->show();

    return app.QApplication::exec();
}