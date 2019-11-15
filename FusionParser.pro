#-------------------------------------------------
#
# Project created by QtCreator 2019-06-17T11:31:29
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = FusionParser
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        fusionparser.cpp \
    qcustomplot.cpp \
    mathfunction.cpp \
    LTAParser/kmlreader.cpp \
    LTAParser/tinyxml2.cpp \
    LTAParser/UTM.cpp \
    PCFusParser/pcfusparser.cpp \
    LOCFusParser/locfusparser.cpp

HEADERS  += fusionparser.h \
    qcustomplot.h \
    mathfunction.h \
    LTAParser/kmlreader.h \
    LTAParser/tinyxml2.h \
    LTAParser/UTM.h \
    PCFusParser/pcfusparser.h \
    LOCFusParser/locfusparser.h

FORMS    += fusionparser.ui
