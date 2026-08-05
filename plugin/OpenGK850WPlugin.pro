QT += widgets

TARGET = libOpenGK850WPlugin
TEMPLATE = lib
CONFIG += plugin  # This is critical for Qt to properly embed metadata!
DEFINES += OPENRGB_PLUGIN

SOURCES += OpenGK850WPlugin.cpp
HEADERS += OpenGK850WPlugin.h
RESOURCES += resources.qrc

unix {
    target.path = /usr/lib/openrgb/plugins
    INSTALLS += target
}

DESTDIR = ..

# Add OpenRGB source root for headers
INCLUDEPATH += /home/truenas_admin/OpenRGB
INCLUDEPATH += /home/truenas_admin/OpenRGB/RGBController
# And the nlohmann json dependency
INCLUDEPATH += /home/truenas_admin/OpenRGB/dependencies/json
# System headers for hidapi
INCLUDEPATH += /usr/include/hidapi

LIBS += -lhidapi-hidraw
