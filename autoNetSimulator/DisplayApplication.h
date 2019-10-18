#ifndef DISPLAYAPPLICATION_H
#define DISPLAYAPPLICATION_H

#include <QApplication>
#include <QAbstractItemView>
#include <qthread.h>

class QItemSelectionModel;
class MainWindow;


class DisplayApplication : public QApplication {
    Q_OBJECT
public:
    DisplayApplication(int &argc, char ** argv);
    virtual ~DisplayApplication();

    static DisplayApplication *instance();

    static MainWindow *mainWindow();

signals:

public slots:

protected:

private:
    MainWindow *_mainWindow;

};

#endif // DISPLAYAPPLICATION_H

