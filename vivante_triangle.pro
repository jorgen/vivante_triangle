TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINE += EGL_API_FB

target.path = $$(IMX_INSTALL_PATH)/egl_example/bin/$$TARGET
INSTALLS += target

LIBS += -lEGL -lGLESv2
SOURCES += main.cpp \
    trianglerenderer.cpp

HEADERS += \
    trianglerenderer.h

