TEMPLATE = aux

feature.path = $$PREFIX/share/qt5/mkspecs/features/
feature.files = $$files(*.prf)

macros.files = macros.sailfish-svg2png
macros.path = /etc/rpm

INSTALLS += feature macros
