#-------------------------------------------------
#
# Project created by QtCreator 2018-10-08T22:26:23
#
#-------------------------------------------------

QT       += core gui
RC_ICONS = myicon.ico
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MyPaintingBoard2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    paintboard.cpp \
    myglwidget.cpp

HEADERS  += mainwindow.h \
    paintboard.h \
    myglwidget.h

FORMS    += mainwindow.ui

RESOURCES += \
    icon.qrc

QT += opengl

win32:LIBS += -lOpengl32 \
                -lglu32 \
                -lglut

