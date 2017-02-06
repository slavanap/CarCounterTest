#-------------------------------------------------
#
# Project created by QtCreator 2017-02-04T10:13:23
#
#-------------------------------------------------

QT += core gui multimediawidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CarCounter
TEMPLATE = app

# OpenCV
msvc {
    INCLUDEPATH += "C:/Program Files/opencv/build/include"
    LIBS += -L"C:/Program Files/opencv/build/x64/vc14/lib"
}
Debug:LIBS += -lopencv_world320d
Release:LIBS += -lopencv_world320

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    mainwindow.h \
    GraphicsItemPolyline.h \
    ImageViewer.h \
    processing.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    GraphicsItemPolyline.cpp \
    ImageViewer.cpp \
    processing.cpp

FORMS += mainwindow.ui
