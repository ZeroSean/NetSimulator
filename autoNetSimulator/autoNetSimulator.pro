#-------------------------------------------------
#
# Project created by QtCreator 2019-10-08T09:59:13
#
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = autoNetSimulator
TEMPLATE = app

DEFINES += QT_MESSAGELOGCONTEXT

INCLUDEPATH += models network views util tools UDP

INCLUDEPATH += $$PWD/armadillo-3.930.0/include


LIBS += -L$$PWD/armadillo-3.930.0/lib/ -lblas_win32_MT

LIBS += -L$$PWD/armadillo-3.930.0/lib/ -llapack_win32_MT

SOURCES += main.cpp \
    views/mainwindow.cpp \
    util/QPropertyModel.cpp \
    tools/OriginTool.cpp \
    models/ViewSettings.cpp \
    tools/ScaleTool.cpp \
    DisplayApplication.cpp \
    tools/RubberBandTool.cpp \
    views/GraphicsView.cpp \
    views/GraphicsWidget.cpp \
    views/MinimapView.cpp \
    views/ViewSettingsWidget.cpp \
    network/Coordinator.cpp \
    network/InstanceAnch.cpp \
    network/InstanceCommon.cpp \
    network/InstanceTag.cpp \
    tools/TagTool.cpp \
    network/RouteTable.cpp \
    UDP/udpserver.cpp

HEADERS  += \
    views/mainwindow.h \
    util/QPropertyModel.h \
    tools/OriginTool.h \
    tools/AbstractTool.h \
    models/ViewSettings.h \
    tools/ScaleTool.h \
    DisplayApplication.h \
    tools/RubberBandTool.h \
    views/GraphicsView.h \
    views/GraphicsWidget.h \
    views/MinimapView.h \
    views/ViewSettingsWidget.h \
    network/Coordinator.h \
    network/decadeviceapi.h \
    network/instance.h \
    network/InstanceAnch.h \
    network/InstanceCommon.h \
    network/RouteTable.h \
    tools/TagTool.h \
    network/InstanceTag.h \
    UDP/udpserver.h \
    log.h

FORMS    += \
    views/mainwindow.ui \
    views/GraphicsWidget.ui \
    views/ViewSettingsWidget.ui

RESOURCES += \
    res/resources.qrc
