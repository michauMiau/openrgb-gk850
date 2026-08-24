QT += widgets

TARGET = OpenGK850WPlugin
TEMPLATE = lib
CONFIG += plugin
DEFINES += OPENRGB_PLUGIN

# CI clones hidapi and OpenRGB into $GITHUB_WORKSPACE, i.e. siblings of
# this repo checkout. Pass OPENRGB_ROOT / HIDAPI_ROOT via qmake args.
isEmpty(OPENRGB_ROOT): OPENRGB_ROOT = $$PWD/../../OpenRGB
isEmpty(HIDAPI_ROOT): HIDAPI_ROOT = $$PWD/../../hidapi

SOURCES += OpenGK850WPlugin.cpp \
    $$HIDAPI_ROOT/windows/hid.c

HEADERS += OpenGK850WPlugin.h

RESOURCES += resources.qrc

INCLUDEPATH += $$HIDAPI_ROOT/hidapi
INCLUDEPATH += $$OPENRGB_ROOT
INCLUDEPATH += $$OPENRGB_ROOT/RGBController
INCLUDEPATH += $$OPENRGB_ROOT/dependencies/json

LIBS += -lsetupapi -lws2_32
QMAKE_LFLAGS += -Wl,--exclude-all-symbols -static-libgcc -static-libstdc++

DESTDIR = ..
