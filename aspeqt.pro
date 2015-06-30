# -------------------------------------------------
# Project created by QtCreator 2009-11-22T14:13:00
# Last Update: Nov 29, 2014
# -------------------------------------------------
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VERSION=\\\"1.00.Preview_7\\\"
TARGET = AspeQt
TEMPLATE = app
CONFIG += qt
QT += core gui network widgets printsupport
CONFIG += mobility
MOBILITY = bearer
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib
SOURCES += main.cpp \
    mainwindow.cpp \
    sioworker.cpp \
    optionsdialog.cpp \
    aboutdialog.cpp \
    diskimage.cpp \
    diskimagepro.cpp \
    folderimage.cpp \
    miscdevices.cpp \
    createimagedialog.cpp \
    diskeditdialog.cpp \
    aspeqtsettings.cpp \
    autoboot.cpp \
    autobootdialog.cpp \
    atarifilesystem.cpp \
    miscutils.cpp \
    textprinterwindow.cpp \
    cassettedialog.cpp \
    docdisplaywindow.cpp \
    bootoptionsdialog.cpp \
    network.cpp \
    logdisplaydialog.cpp
win32:LIBS += -lwinmm
unix:LIBS += -lz
win32:SOURCES += serialport-win32.cpp
unix:SOURCES += serialport-unix.cpp
HEADERS += mainwindow.h \
    serialport.h \
    sioworker.h \
    optionsdialog.h \
    aboutdialog.h \
    diskimage.h \
    diskimagepro.h \
    folderimage.h \
    miscdevices.h \
    createimagedialog.h \
    diskeditdialog.h \
    aspeqtsettings.h \
    autoboot.h \
    autobootdialog.h \
    atarifilesystem.h \
    miscutils.h \
    textprinterwindow.h \
    cassettedialog.h \
    docdisplaywindow.h \
    bootoptionsdialog.h \
    network.h \
    logdisplaydialog.h \

win32:HEADERS += serialport-win32.h
unix:HEADERS += serialport-unix.h
FORMS += mainwindow.ui \
    optionsdialog.ui \
    aboutdialog.ui \
    createimagedialog.ui \
    diskeditdialog.ui \
    autobootdialog.ui \
    textprinterwindow.ui \
    cassettedialog.ui \
    docdisplaywindow.ui \
    bootoptionsdialog.ui \
    logdisplaydialog.ui
RESOURCES += icons.qrc \
    atarifiles.qrc \
    i18n.qrc \
    documentation.qrc \
    images.qrc
OTHER_FILES += \
    license.txt \
    history.txt \
    atascii_read_me.txt \
    AspeQt.rc \
    about.html \
    compile.txt \
TRANSLATIONS = i18n/aspeqt_pl.ts \
               i18n/aspeqt_tr.ts \
               i18n/aspeqt_ru.ts \
               i18n/aspeqt_sk.ts \
               i18n/aspeqt_de.ts \
               i18n/aspeqt_es.ts \
               i18n/qt_pl.ts \
               i18n/qt_tr.ts \
               i18n/qt_ru.ts \
               i18n/qt_sk.ts \
               i18n/qt_de.ts \
               i18n/qt_es.ts

RC_FILE = AspeQt.rc \




