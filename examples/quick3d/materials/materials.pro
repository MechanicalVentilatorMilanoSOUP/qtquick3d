QT += quick quick3d-private


target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/materials
INSTALLS += target

SOURCES += \
    main.cpp

RESOURCES += \
    qml.qrc \
    materials.qrc
