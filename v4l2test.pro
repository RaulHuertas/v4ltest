QT -= core
QT -= gui

TARGET = v4l2test
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++14
TEMPLATE = app

CONFIG += link_pkgconfig
PKGCONFIG += opencv

SOURCES += main.cpp \
    main1.cpp \
    transmitirporudp.cpp

HEADERS += \
    transmitirporudp.hpp

