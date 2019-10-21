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

ViewSettingsWidget::ViewSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewSettingsWidget),
    _floorplanOpen(false),
    _ancConfigSelect(false)
{
    ui->setupUi(this);

    QObject::connect(ui->floorplanOpen_pb, SIGNAL(clicked(bool)), this, SLOT(floorplanOpenClicked()));
    QObject::connect(ui->ancConfigSelect_pb, SIGNAL(clicked(bool)), this, SLOT(ancConfigSelectClicked()));

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

    QObject::connect(DisplayApplication::viewSettings(), SIGNAL(showSave(bool)), this, SLOT(showSave(bool)));
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
    mapper->toFirst();

    QObject::connect(ui->floorplanFlipX_cb, SIGNAL(clicked(bool)), mapper, SLOT(submit()));
    QObject::connect(ui->floorplanFlipY_cb, SIGNAL(clicked(bool)), mapper, SLOT(submit()));
    QObject::connect(ui->gridShow, SIGNAL(clicked(bool)), mapper, SLOT(submit()));
    QObject::connect(ui->showOrigin, SIGNAL(clicked(bool)), mapper, SLOT(submit()));

    ui->showTagHistory->setChecked(true);
    ui->tabWidget->setCurrentIndex(0);

    DisplayApplication::graphicsWidget()->communicateRangeValue(ui->communicateRange->value());
    DisplayApplication::graphicsWidget()->zone2Value(ui->zone2->value());

}

ViewSettingsWidget::~ViewSettingsWidget() {
    delete ui;
}

int ViewSettingsWidget::applyFloorplanPic(const QString &path) {
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
        QString path = QFileDialog::getOpenFileName(this, "Open Bitmap", QString(), "Image (*.png *.jpg *.jpeg *.bmp)");

        if(path.isNull()) {
            return;
        }

        if(applyFloorplanPic(path) == 0) {
            DisplayApplication::viewSettings()->setFloorplanShow(true);
            DisplayApplication::viewSettings()->setFloorplanPath(path);
        }

        _floorplanOpen = true;
        ui->floorplanOpen_pb->setText("Clear");
    } else {
        DisplayApplication::viewSettings()->setFloorplanShow(false);
        DisplayApplication::viewSettings()->clearSettings();

        _floorplanOpen = false;
        ui->floorplanOpen_pb->setText("Open");
        ui->floorplanFlipX_cb->setChecked(false);
        ui->floorplanFlipY_cb->setChecked(false);
        ui->floorplanXScale_sb->setValue(0.0);
        ui->floorplanYScale_sb->setValue(0.0);
        ui->floorplanXOff_sb->setValue(0);
        ui->floorplanYOff_sb->setValue(0);
        ui->floorplanPath_lb->setText("");
    }
}

void ViewSettingsWidget::ancConfigSelectClicked() {
    if(_ancConfigSelect == false) {
        //参数：父组件、对话框标题、默认打开目录、后缀名过滤器
        QString path = QFileDialog::getOpenFileName(this, "Open txt", "./", "Txt (*.txt)");

        if(path.isNull()) {
            return;
        }

        QFile file(path);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug(qPrintable(QString("Error: cannot read file (%1) %2").arg(path).arg(file.errorString())));
            return;
        } else {
            QTextStream stream(&file);
            QString line = "";
            while(!(line = stream.readLine()).isNull()) {
                qDebug() << line;
            }
        }



        _ancConfigSelect = true;
        ui->ancConfigLable->setText("anc file:" + path);

    }
}

void ViewSettingsWidget::showGridOrigin(bool grid, bool orig) {
    ui->gridShow->setChecked(grid);
    ui->showOrigin->setChecked(orig);
}

void ViewSettingsWidget::gridShowClicked() {
    DisplayApplication::viewSettings()->setShowGrid(ui->gridShow->isChecked());
}

void ViewSettingsWidget::originShowClicked() {
    DisplayApplication::viewSettings()->setShowOrigin(ui->showOrigin->isChecked());
}

void ViewSettingsWidget::tagHistoryNumberValueChanged(int value) {
    DisplayApplication::graphicsWidget()->tagHistoryNumber(value);
}

void ViewSettingsWidget::communicateRangeEditFinished() {
    DisplayApplication::graphicsWidget()->communicateRangeValue(ui->communicateRange->value());
}

void ViewSettingsWidget::zone2EditFinished() {
    DisplayApplication::graphicsWidget()->zone2Value(ui->zone2->value());
}

void ViewSettingsWidget::communicateRangeChanged(double value) {
    DisplayApplication::graphicsWidget()->communicateRangeValue(value);
}

void ViewSettingsWidget::zone2ValueChanged(double value) {
    DisplayApplication::graphicsWidget()->zone2Value(value);
}

void ViewSettingsWidget::setTagHistory(int h) {
    ui->tagHistoryN->setValue(h);
}

void ViewSettingsWidget::tagHistoryShowClicked() {
    DisplayApplication::graphicsWidget()->setShowTagHistory(ui->showTagHistory->isChecked());
}

void ViewSettingsWidget::loggingClicked() {
    if(_logging == false) {
        _logging = true;
        ui->logging_pb->setText("Stop");
        ui->label_logingstatus->setText("Logging enabled.");
        //ui->label_logfile(DisplayApplication::comsumer()->getLogFilePath());
    } else {
        ui->logging_pb->setText("Start");
        ui->label_logingstatus->setText("Logging disabled.");
        ui->label_logfile->setText("");
        ui->saveFP->setChecked(false);
        _logging = false;
    }
}

void ViewSettingsWidget::saveFPClicked() {
    DisplayApplication::viewSettings()->setSaveFP(ui->saveFP->isChecked());

    if(ui->saveFP->isChecked()) {
        emit saveViewSettings();
    }
}

void ViewSettingsWidget::showSave(bool arg) {
    ui->saveFP->setChecked(arg);
}

void ViewSettingsWidget::originClicked() {
    OriginTool *tool = new OriginTool(this);
    QObject::connect(tool, SIGNAL(done(bool)), tool, SLOT(deleteLater()));
    DisplayApplication::graphicsView()->setTool(tool);
}

void ViewSettingsWidget::scaleClicked() {
    ScaleTool *tool = NULL;

    if(QObject::sender() == ui->scaleX_pb) {
        tool = new ScaleTool(ScaleTool::XAxis, this);
    } else if(QObject::sender() == ui->scaleY_pb) {
        tool = new ScaleTool(ScaleTool::YAxis, this);
    }

    QObject::connect(tool, SIGNAL(done(bool)), tool, SLOT(deleteLater()));
    DisplayApplication::graphicsView()->setTool(tool);
}




















