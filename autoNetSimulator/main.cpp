#include "DisplayApplication.h"
#include "mainwindow.h"
#include <QApplication>
#include <qdir.h>
#include "log.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QSet<quint16> >("QSet<quint16>");

    if(argc > 2) {
        //设定日志级别，比如设置日志级别为2： xxx.exe log_file 2
        QT_LOG::logInit(QString(argv[1]).split(QDir::separator()).last() + ".log",
                QString(argv[2]).toUInt());
    } else {
        QT_LOG::logInit();
    }

    DisplayApplication app(argc, argv);

    app.mainWindow()->show();

    return app.QApplication::exec();
}
