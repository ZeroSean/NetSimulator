#include "ViewSettings.h"

/*
管理floorplan的相关设置
*/
ViewSettings::ViewSettings(QObject *parent)
    : QObject(parent),
      _gridWidth(0.5),
      _gridHeight(0.5),
      _floorplanXFlip(false),
      _floorplanYFlip(false),
      _floorplanXScale(100),
      _floorplanYScale(100),
      _floorplanXOffset(0),
      _floorplanYOffset(0),
      _showOrigin(true),
      _showGrid(true),
      _floorplanShow(false),
      _floorplanPath(""),
      _ancConfigFilePath("")
{
    QObject::connect(this, SIGNAL(gridWidthChanged(double)), this, SLOT(viewSettingsChanged()));
    QObject::connect(this, SIGNAL(gridHeightChanged(double)), this, SLOT(viewSettingsChanged()));

    QObject::connect(this, SIGNAL(floorplanXFlipChanged(bool)), this, SLOT(viewSettingsChanged()));
    QObject::connect(this, SIGNAL(floorplanYFlipChanged(bool)), this, SLOT(viewSettingsChanged()));

    QObject::connect(this, SIGNAL(floorplanXScaleChanged(double)), this, SLOT(viewSettingsChanged()));
    QObject::connect(this, SIGNAL(floorplanYScaleChanged(double)), this, SLOT(viewSettingsChanged()));

    QObject::connect(this, SIGNAL(floorplanXOffsetChanged(double)), this, SLOT(viewSettingsChanged()));
    QObject::connect(this, SIGNAL(floorplanYOffsetChanged(double)), this, SLOT(viewSettingsChanged()));

    QObject::connect(this, SIGNAL(showGridChanged(bool)), this, SLOT(viewSettingsChanged()));
    QObject::connect(this, SIGNAL(showOriginChanged(bool)), this, SLOT(viewSettingsChanged()));

    QObject::connect(this, SIGNAL(floorplanPixmapChanged()), this, SLOT(viewSettingsChanged()));
}

bool ViewSettings::originShow() {
    return _showOrigin;
}

bool ViewSettings::gridShow() {
    return _showGrid;
}

void ViewSettings::setSaveFP(bool set) {
    _floorplanSave = set;

    emit showSave(set);
}

void ViewSettings::setShowOrigin(bool set) {
    _showOrigin = set;

    emit showGO(_showGrid, _showOrigin);
}

void ViewSettings::setShowGrid(bool set) {
    _showGrid = set;

    emit showGO(_showGrid, _showOrigin);
    emit floorplanChanged();
}

double ViewSettings::gridHeight() const {
    return _gridHeight;
}

double ViewSettings::gridWidth() const {
    return _gridWidth;
}

bool ViewSettings::floorplanXFlip() const {
    return _floorplanXFlip;
}

bool ViewSettings::floorplanYFlip() const {
    return _floorplanYFlip;
}

double ViewSettings::floorplanXScale() const {
    return _floorplanXScale;
}

double ViewSettings::floorplanYScale() const {
    return _floorplanYScale;
}

double ViewSettings::floorplanXOffset() const {
    return _floorplanXOffset;
}

double ViewSettings::floorplanYOffset() const {
    return _floorplanYOffset;
}

const QPixmap &ViewSettings::floorplanPixmap() const {
    return _floorplanPixmap;
}

const QString &ViewSettings::getFloorplanPath() {
    return _floorplanPath;
}

bool ViewSettings::floorplanSave() const {
    return _floorplanSave;
}

QTransform ViewSettings::floorplanTransform() const {
    return _floorplanTransform;
}

void ViewSettings::setGridWidth(double arg) {
    if(_gridWidth != arg) {
        _gridWidth = arg;
        emit gridWidthChanged(arg);
    }
}

void ViewSettings::setGridHeight(double arg) {
    if(_gridHeight != arg) {
        _gridHeight = arg;
        emit gridHeightChanged(arg);
    }
}

void ViewSettings::setFloorplanXFlip(bool arg) {
    if(_floorplanXFlip != arg) {
        _floorplanXFlip = arg;
        emit floorplanXFlipChanged(arg);
    }
}

void ViewSettings::setFloorplanYFlip(bool arg) {
    if(_floorplanYFlip != arg) {
        _floorplanYFlip = arg;
        emit floorplanYFlipChanged(arg);
    }
}

void ViewSettings::setFloorplanXScale(double arg) {
    if(_floorplanXScale != arg) {
        _floorplanXScale = arg;
        emit floorplanXScaleChanged(arg);
    }
}

void ViewSettings::setFloorplanYScale(double arg) {
    if(_floorplanYScale != arg) {
        _floorplanYScale = arg;
        emit floorplanYScaleChanged(arg);
    }
}

void ViewSettings::setFloorplanXOffset(double arg) {
    if(_floorplanXOffset != arg) {
        _floorplanXOffset = arg;
        emit floorplanXOffsetChanged(arg);
    }
}

void ViewSettings::setFloorplanYOffset(double arg) {
    if(_floorplanYOffset != arg) {
        _floorplanYOffset = arg;
        emit floorplanYOffsetChanged(arg);
    }
}

void ViewSettings::setFloorplanPixmap(const QPixmap &arg) {
    _floorplanPixmap = arg;
    setFloorplanXFlip(true);
    emit floorplanPixmapChanged();
}

void ViewSettings::setFloorplanPath(const QString &arg) {
    _floorplanPath = arg;
}

void ViewSettings::setFloorplanPathN(void) {
    if(!_floorplanPath.isNull()) {
        _floorplanShow = true;
        emit setFloorplanPic();
    }
}

void ViewSettings::setFloorplanShow(bool arg) {
    _floorplanShow = arg;
    emit floorplanChanged();
}

bool ViewSettings::getFloorplanShow(void) {
    return _floorplanShow;
}

void ViewSettings::clearSettings(void) {
    _floorplanXScale = 0;
    _floorplanYScale = 0;
    _floorplanXOffset = 0;
    _floorplanYOffset = 0;
    _floorplanXFlip = false;
    _floorplanYFlip = false;
}

void ViewSettings::viewSettingsChanged() {
    QTransform t;
    double xscale = floorplanXScale();
    double yscale = floorplanYScale();
    double xoffset = floorplanXOffset();
    double yoffset = floorplanYOffset();

    if(!floorplanPixmap().isNull() && (xscale != 0) && (yscale != 0)) {
        if(floorplanXFlip()) {
            t.scale(1, -1);
        }
        if(floorplanYFlip()) {
            t.scale(-1, 1);
        }

        t.scale(1.0 / xscale, 1.0 / yscale);
        t.translate(-xoffset, -yoffset);
    }

    _floorplanTransform = t;

    emit floorplanChanged();
}

void ViewSettings::setAncConfigFilePath(const QString &path) {
    if(!path.isNull()) {
        if(_ancConfigFilePath != path) {
            _ancConfigFilePath = path;
            emit ancConfigFileChanged();
        }
    }
}

const QString &ViewSettings::getAncConfigFilePath(void) {
    return _ancConfigFilePath;
}



