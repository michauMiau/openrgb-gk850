QT += widgets

TARGET = OpenGK850WPlugin
TEMPLATE = lib
CONFIG += plugin
DEFINES += OPENRGB_PLUGIN HID_API_NO_STATIC_LIB

SOURCES += OpenGK850WPlugin.cpp \
    ../hidapi/windows/hid.cpp

HEADERS += OpenGK850WPlugin.h \
    ../hidapi/hidapi/hidapi.h

RESOURCES += resources.qrc

INCLUDEPATH += $$PWD/../hidapi/hidapi
INCLUDEPATH += $$GITHUB_WORKSPACE/OpenRGB
INCLUDEPATH += $$GITHUB_WORKSPACE/OpenRGB/RGBController
INCLUDEPATH += $$GITHUB_WORKSPACE/OpenRGB/dependencies/json

LIBS += -lsetupapi -lws2_32

DESTDIR = ..
