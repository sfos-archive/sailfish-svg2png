TEMPLATE = aux

feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
feature.files = $$files(*.prf)

macros.files = macros.sailfish-svg2png
macros.path = /usr/lib/rpm/macros.d

INSTALLS += feature macros
