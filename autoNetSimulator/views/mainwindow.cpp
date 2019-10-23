#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "DisplayApplication.h"
#include "ViewSettings.h"

#include <QShortcut>
#include <QSettings>
#include <QDebug>
#include <QMessageBox>
#include <QDomDocument>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _showMainToolBar = false;

    {
        QWidget *empty = new QWidget(this);
        empty->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        ui->mainToolBar->addWidget(empty);
    }

    _anchorConfigAction = new QAction(tr("&Connection Configuration"), this);

    createPopupMenu(ui->viewMenu);

    QObject::connect(DisplayApplication::instance(), SIGNAL(aboutToQuit()), SLOT(saveSettings()));

    ui->helpMenu->addAction(ui->actionAbout);
    connect(ui->actionAbout, SIGNAL(triggered(bool)), SLOT(onAboutAction()));

    _infoLabel = new QLabel(parent);

    ui->viewSettings_dw->close();
    ui->minimap_dw->close();

    connect(ui->minimap_dw->toggleViewAction(), SIGNAL(triggered(bool)), SLOT(onMiniMapView()));

    DisplayApplication::connectReady(this, "onReady()");
}

void MainWindow::onReady() {
    QObject::connect(graphicsWidget(), SIGNAL(setTagHistory(int)), viewSettingsWidget(), SLOT(setTagHistory(int)));
    QObject::connect(viewSettingsWidget(), SIGNAL(saveViewSettings()), this, SLOT(saveViewConfigSettings()));

    loadSettings();

    if(_showMainToolBar) {
        ui->mainToolBar->show();
    } else {
        ui->mainToolBar->hide();
    }

    ui->viewSettings_dw->show();

#ifdef QT_DEBUG
    ui->mainToolBar->show();
#else

#endif
}

MainWindow::~MainWindow() {
    delete ui;
}

GraphicsWidget *MainWindow::graphicsWidget() {
    return ui->graphicsWidget;
}

ViewSettingsWidget *MainWindow::viewSettingsWidget() {
    return ui->viewSettings_w;
}

QMenu *MainWindow::createPopupMenu() {
    return createPopupMenu(new QMenu);
}

QMenu *MainWindow::createPopupMenu(QMenu *menu) {
    menu->addAction(ui->viewSettings_dw->toggleViewAction());
    menu->addAction(ui->minimap_dw->toggleViewAction());

    return menu;
}

void MainWindow::onAnchorConfigAction() {
    ui->mainToolBar->show();
}

void MainWindow::onMiniMapView() {
    QString path = DisplayApplication::viewSettings()->getFloorplanPath();

    if(path == "") {
        ui->minimap_dw->close();
        QMessageBox::warning(NULL, tr("Error"), "No floorplan loaded. Please upload floorplan to use mini-map");
    }
}

void MainWindow::onAboutAction() {
    _infoLabel->setText(tr("Invoked <b>Help|About</b>"));
    QMessageBox::about(this, tr("About"),
                       tr("<b>NetSimulator</b>"
                          "<br>version 0.1 (" __DATE__
                          ")"));
}

void MainWindow::loadSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    QVariant state = s.value("window-state");
    if(state.convert(QVariant::ByteArray)) {
        this->restoreState(state.toByteArray());
    }

    QVariant geometry = s.value("window-geometry");
    if(state.convert(QVariant::ByteArray)) {
        this->restoreGeometry(geometry.toByteArray());
    } else {
        this->showMaximized();
    }
    s.endGroup();

    loadConfigFile("./view_config.xml");
    graphicsWidget()->loadConfigFile("./tag_config.xml");
}

void MainWindow::saveSettings() {
    QSettings s;
    s.beginGroup("MainWindow");
    s.setValue("window-state", this->saveState());
    s.setValue("window-geometry", this->saveGeometry());
    s.endGroup();

    saveConfigFile("./view_config.xml", "view_cfg");
    graphicsWidget()->saveConfigFile("./tag_config.xml");
}

void MainWindow::saveViewConfigSettings() {
    saveConfigFile("./view_config.xml", "view_cfg");
}

void MainWindow::loadConfigFile(QString filename) {
    QFile file(filename);

    if(!file.open(QFile::ReadWrite | QFile::Text)) {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }

    QDomDocument doc;
    doc.setContent(&file, false);

    QDomElement root = doc.documentElement();

    qDebug() << root.tagName();

    if(root.tagName() == "config") {
        QDomNode n = root.firstChild();
        while(!n.isNull()) {
            QDomElement e = n.toElement();
            if(!e.isNull()) {
                if(e.tagName() == "view_cfg") {
                    ViewSettings *vs = DisplayApplication::viewSettings();
                    vs->setGridWidth((e.attribute("gridW", "")).toDouble());
                    vs->setGridHeight((e.attribute("gridH", "")).toDouble());
                    vs->setShowGrid(((e.attribute("gridS", "")).toInt() == 1) ? true : false);
                    vs->setShowOrigin(((e.attribute("originS", "")).toInt() == 1) ? true : false);
                    vs->setFloorplanPath(e.attribute("fplan", ""));
                    vs->setFloorplanXOffset((e.attribute("offsetX", "")).toDouble());
                    vs->setFloorplanYOffset((e.attribute("offsetY", "")).toDouble());
                    vs->setFloorplanXScale((e.attribute("scaleX", "")).toDouble());
                    vs->setFloorplanYScale((e.attribute("scaleY", "")).toDouble());
                    vs->setFloorplanXFlip((e.attribute("filpX", "")).toInt());
                    vs->setFloorplanYFlip((e.attribute("filpY", "")).toInt());

                    vs->setFloorplanPathN();
                    vs->setSaveFP(((e.attribute("saveFP", "")).toInt() == 1) ? true : false);

                    vs->setAncConfigFilePath(e.attribute("ancPath", ""));
                }
            }
            n = n.nextSibling();
        }
    }

    file.close();
}

void MainWindow::saveConfigFile(QString filename, QString cfg) {
    QFile file(filename);

    QDomDocument doc;
    QDomElement info = doc.createElement("config");
    doc.appendChild(info);

    qDebug() << info.tagName();

    if(cfg == "view_cfg") {
        QDomElement cn = doc.createElement("view_cfg");
        ViewSettings *vs = DisplayApplication::viewSettings();

        cn.setAttribute("gridW", QString::number(vs->gridWidth(), 'g', 3));
        cn.setAttribute("gridH", QString::number(vs->gridHeight(), 'g', 3));
        cn.setAttribute("gridS", QString::number((vs->gridShow() == true) ? 1 : 0));
        cn.setAttribute("originS", QString::number((vs->originShow() == true) ? 1 : 0));
        cn.setAttribute("saveFP", QString::number((vs->floorplanSave() == true) ? 1 : 0));

        if(vs->floorplanSave()) {
            cn.setAttribute("flipX", QString::number(vs->floorplanXFlip(), 10));
            cn.setAttribute("flipY", QString::number(vs->floorplanYFlip(), 10));
            cn.setAttribute("scaleX", QString::number(vs->floorplanXScale(), 'g', 3));
            cn.setAttribute("scaleY", QString::number(vs->floorplanYScale(), 'g', 3));
            cn.setAttribute("offsetX", QString::number(vs->floorplanXOffset(), 'g', 3));
            cn.setAttribute("offsetY", QString::number(vs->floorplanYOffset(), 'g', 3));
            cn.setAttribute("fplan", vs->getFloorplanPath());
        }

        cn.setAttribute("ancPath", vs->getAncConfigFilePath());

        info.appendChild(cn);
    }

    file.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream ts(&file);
    ts << doc.toString();

    qDebug() << doc.toString();

    file.close();
}

void MainWindow::statusBarMessage(QString status) {
    ui->statusBar->showMessage(status);
}










