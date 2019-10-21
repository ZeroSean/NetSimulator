#ifndef DISPLAYAPPLICATION_H
#define DISPLAYAPPLICATION_H

#include <QApplication>
#include <QAbstractItemView>
#include <qthread.h>

class QItemSelectionModel;
class MainWindow;
class ViewSettings;
class GraphicsWidget;
class GraphicsView;


/**
 * 单例模式
 */
class DisplayApplication : public QApplication {
    Q_OBJECT

public:
    explicit DisplayApplication(int &argc, char ** argv);
    virtual ~DisplayApplication();

    static DisplayApplication *instance();
    static ViewSettings *viewSettings();

    static MainWindow *mainWindow();

    static GraphicsWidget *graphicsWidget();
    static GraphicsView *graphicsView();

    /**
    * Call \a member of \a receiver once initialization is complete.
    * The function is either called reight away if initialization is already done, or connected to the ready() signal.
    * The method must be registered within Qt's meta object system, using Q_INVOKABLE, or declaring it as a slot.
    */
    static void connectReady(QObject *receiver, const char *member, Qt::ConnectionType type = Qt::AutoConnection);

signals:
    /**
    * Emitted when the inizialization is complete.
    * Because this signal is only emitted once at application startup, the connectReady() should be used instead.
    */
    void ready();

public slots:

protected:

private:
    MainWindow *_mainWindow;

    ViewSettings *_viewSettings;

    bool _ready;

};

#endif // DISPLAYAPPLICATION_H

