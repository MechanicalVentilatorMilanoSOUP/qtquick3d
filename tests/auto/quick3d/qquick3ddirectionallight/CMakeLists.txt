# Generated from qquick3ddirectionallight.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3ddirectionallight Test:
#####################################################################

qt_add_test(qquick3ddirectionallight
    SOURCES
        tst_qquick3ddirectionallight.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3ddirectionallight.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
