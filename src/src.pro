TEMPLATE = app
TARGET = sailfish_svg2png
TARGETPATH = /usr/bin

QT += gui svg

CONFIG += link_pkgconfig

PKGCONFIG += librsvg-2.0 glib-2.0 cairo libpng

SOURCES += main.cpp

target.path = $$TARGETPATH

INSTALLS += target
