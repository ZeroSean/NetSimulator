#include "ViewSettingsWidget.h"
#include "ui_viewsettingswidget.h"

#include "DisplayApplication.h"
#include "QPropertyModel.h"
#include "ViewSettings.h"
#include "OriginTool.h"
#include "ScaleTool.h"
#include "GraphicsView.h"
#include "GraphicsWidget.h"

#include <QFileDialog>
#include <QMessageBox>

ViewSettingsWidget::ViewSettingsWidget(QWidget *parent = 0) :
    QWidget(parent),
    ui(new Ui::ViewSettingsWidget),
    _floorplanOpen(false)
{
    ui->setupUi(this);

    QObject::connect(ui->floorplanOpen_pb, SIGNAL(clicked(bool)), this, SLOT(floorplanOpenClicked()));

    QObject::connect(ui->scaleX_pb, SIGNAL(clicked(bool)), this, SLOT(scaleClicked()));
    QObject::connect(ui->scaleY_pb, SIGNAL(clicked(bool)), this, SLOT(scaleClicked()));
    QObject::connect(ui->origin_pb, SIGNAL(clicked(bool)), this, SLOT(originClicked()));

    QObject::connect(ui->saveFP, SIGNAL(clicked(bool)), this, SLOT(saveFPClicked()));
    QObject::connect(ui->gridShow, SIGNAL(clicked(bool)), this, SLOT(gridShowClicked()));
    QObject::connect(ui->showOrigin, SIGNAL(clicked(bool)), this, SLOT(originShowClicked()));
    QObject::connect(ui->showTagHistory, SIGNAL(clicked(bool)), this, SLOT(tagHistoryShowClicked()));

    QObject::connect(ui->communicateRange, SIGNAL(editingFinished()), this, SLOT(communicateRangeEditFinished()));
    QObject::connect(ui->communicateRange, SIGNAL(valueChanged(double)), this, SLOT(communicateRangeChanged(double)));
    QObject::connect(ui->zone2, SIGNAL(editingFinished()), this, SLOT(zone2EditFinished()));
    QObject::connect(ui->zone2, SIGNAL(valueChanged(double)), this, SLOT(zone2ValueChanged(double)));

    QObject::connect(ui->tagHistoryN, SIGNAL(valueChanged(int)), this, SLOT(tagHistoryNumberValueChanged(int)));

    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(showSave(bool)), this, SLOT(showSave()));
    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(showGO(bool, bool)), this, SLOT(showGridOrigin(bool,bool)));
    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(setFloorplanPic()), this, SLOT(getFloorplanPic()));

    QObject::connect(ui->logging_pb, SIGNAL(clicked(bool)), this, SLOT(loggingClicked()));

    QObject::connect(ui->scaleX_pb, SIGNAL(clicked(bool)), this, SLOT(scaleClicked()));

    _logging = false;

    ui->label_logfile->setText("");
    if(_logging) {
        ui->logging_pb->setText("Stop");
        ui->label_logingstatus->setText("Logging anabled.");
    } else {
        ui->logging_pb->setText("Start");
        ui->label_logingstatus->setText("Logging disabled.");
    }

    DisplayApplication::connectReady(this, "onReady()");
}

void ViewSettingsWidget::onReady() {
    QPropertyDataWidgetMapper *mapper = QPropertyModel::newMapper(DisplayApplication::viewSettings(), this);

    mapper->addMapping(ui->gridWidth_sb, "gridWidth");
    mapper->addMapping(ui->gridHeight_sb, "gridHeight");

    mapper->addMapping(ui->floorplanFlipX_cb, "floorplanXFlip", "checked");
    mapper->addMapping(ui->floorplanFlipY_cb, "floorplanYFlip", "checked");
    mapper->addMapping(ui->gridShow, "showGrid", "checked");
    mapper->addMapping(ui->showOrigin, "showOrigin", "checked");

    mapper->addMapping(ui->floorplanXOff_sb, "floorplanXOffset");
    mapper->addMapping(ui->floorplanYOff_sb, "floorplanYOffset");
    mapper->addMapping(ui->floorplanXScale_sb, "floorplanXScale");
    mapper->addMapping(ui->floorplanYScale_sb, "floorplanYScale");

    QObject::connect(ui->floorplanFlipX_cb, SIGNAL(clicked(bool)), mapper, SLOT(submit()));
    QObject::connect(ui->floorplanFlipY_cb, SIGNAL(clicked(bool)), mapper, SLOT(submit()));
    QObject::connect(ui->gridShow, SIGNAL(clicked(bool)), mapper, SLOT(submit()));
    QObject::connect(ui->showOrigin, SIGNAL(clicked(bool)), mapper, SLOT(submit()));

    ui->showTagHistory->setChecked(true);
    ui->tabWidget->setCurrentIndex(0);

    DisplayApplication::graphicsWidget()->CommunicateRangeValue(ui->communicateRange->value());
    DisplayApplication::graphicsWidget()->zone2Value(ui->zone2->value());

}

ViewSettingsWidget::~ViewSettingsWidget() {
    delete ui;
}

int ViewSettingsWidget::applyFloorplanPic(const QString *path) {
    QPixmap pm(path);

    if(pm.isNull()) {
        return -1;
    }

    ui->floorplanPath_lb->setText(QFileInfo(path).fileName());
    DisplayApplication::viewSettings()->setFloorplanPixmap(pm);
    _floorplanOpen = true;
    ui->floorplanOpen_pb->setText("Clear");

    return 0;
}

void ViewSettingsWidget::getFloorplanPic() {
    applyFloorplanPic(DisplayApplication::viewSettings()->getFloorplanPath());
}

void ViewSettingsWidget::floorplanOpenClicked() {
    if(_floorplanOpen == false) {
        QString path = QFileDialog::getOpenFileName(this, "Open Bitmap", QString(), "Image (*.png, *.jpg, *.jpeg, *.bmp)");

        if(path.isNull()) {
            return;
        }

        if(applyFloorplanPic(path) == 0) {
            DisplayApplication::viewSettings()->floorplanShow(true);
            DisplayApplication::viewSettings()->setFloorplanPath(path);
        }

        _floorplanOpen = true;
        ui->floorplanOpen_pb->setText("Clear");
    } else {
        DisplayApplication::viewSe

    }
}









