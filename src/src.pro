TEMPLATE = app
TARGET = sailfish_svg2png
TARGETPATH = /usr/bin

QT += gui svg

SOURCES += main.cpp

target.path = $$TARGETPATH

INSTALLS += target
