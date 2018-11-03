# -------------------------------------------------
# Project created by QtCreator 2009-11-22T14:13:00
# Last Update: Nov 29, 2014
# -------------------------------------------------
#
# Copyright 2015,2016 Joseph Zatarski
#
# This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
# However, the years for these copyrights are unfortunately unknown. If you
# know the specific year(s) please let the current maintainer know.
#
#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT
DEFINES += VERSION=\\\"r4.2.1\\\"
TARGET = RespeQt
TEMPLATE = app
CONFIG += qt
QT += core gui network widgets printsupport serialport
CONFIG += mobility
MOBILITY = bearer
INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib
SOURCES += main.cpp \
    mainwindow.cpp \
    sioworker.cpp \
    optionsdialog.cpp \
    aboutdialog.cpp \
    diskimage.cpp \
    folderimage.cpp \
    miscdevices.cpp \
    createimagedialog.cpp \
    diskeditdialog.cpp \
    autoboot.cpp \
    autobootdialog.cpp \
    atarifilesystem.cpp \
    miscutils.cpp \
    textprinterwindow.cpp \
    cassettedialog.cpp \
    docdisplaywindow.cpp \
    bootoptionsdialog.cpp \
    network.cpp \
    logdisplaydialog.cpp \
    respeqtsettings.cpp \
    drivewidget.cpp \
    pclink.cpp \
    infowidget.cpp \
    Cpu6502.cpp \
    Crc16.cpp \
    Fdc.cpp \
    Motor.cpp \
    Ram.cpp \
    Riot810.cpp \
    Riot1050.cpp \
    Riot6532.cpp \
    RiotDevices.cpp \
    Rom.cpp \
    RomProvider.cpp \
    Sio.cpp \
    Track.cpp \
    Atari810.cpp \
    Atari1050.cpp \
    AtariDrive.cpp \
    FirmwareDiskImage.cpp \
    Atari810Happy.cpp \
    RamUnderRomArchiver.cpp \
    Atari1050Happy.cpp \
    Atari1050Speedy.cpp \
    Atari1050Turbo.cpp \
    Atari1050Duplicator.cpp \
    RomBankSwitchTurbo.cpp \
    diskimagepro.cpp \
    diskimageatx.cpp \
    diskimageatr.cpp \
    diskimagescp.cpp \
    disassembly810.cpp \
    disassembly1050.cpp \
    serialport-test.cpp
win32:LIBS += -lwinmm -lz
unix:LIBS += -lz
win32:SOURCES += serialport-win32.cpp
unix:SOURCES += serialport-unix.cpp
HEADERS += mainwindow.h \
    serialport.h \
    sioworker.h \
    optionsdialog.h \
    aboutdialog.h \
    diskimage.h \
    folderimage.h \
    miscdevices.h \
    createimagedialog.h \
    diskeditdialog.h \
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
    respeqtsettings.h \
    drivewidget.h \
    pclink.h \
    infowidget.h \
    Chip.hpp \
    Cpu6502.hpp \
    Crc16.hpp \
    Fdc.hpp \
    Motor.hpp \
    Ram.hpp \
    Riot810.hpp \
    Riot1050.hpp \
    Riot6532.hpp \
    RiotDevices.hpp \
    Rom.hpp \
    RomProvider.hpp \
    Sio.hpp \
    Track.hpp \
    Emulator.h \
    Atari810.hpp \
    Atari1050.hpp \
    AtariDrive.hpp \
    FirmwareDiskImage.hpp \
    Atari810Happy.hpp \
    RamUnderRomArchiver.hpp \
    Atari1050Happy.hpp \
    Atari1050Speedy.hpp \
    Atari1050Turbo.hpp \
    Atari1050Duplicator.hpp \
    RomBankSwitchTurbo.hpp \
    disassembly810.h \
    disassembly1050.h \
    infowidget.h \
    serialport-test.h

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
    logdisplaydialog.ui \
    drivewidget.ui \
    infowidget.ui
RESOURCES += icons.qrc \
    atarifiles.qrc \
    i18n.qrc \
    documentation.qrc
OTHER_FILES += \
    license.txt \
    history.txt \
    atascii_read_me.txt \
    RespeQt.rc \
    about.html \
    compile.txt \
TRANSLATIONS = \
               i18n/respeqt_de.ts \
               i18n/respeqt_es.ts \
               i18n/qt_pl.ts \
               i18n/qt_tr.ts \
               i18n/qt_ru.ts \
               i18n/qt_sk.ts \
               i18n/qt_de.ts \
               i18n/qt_es.ts \
    i18n/respeqt_de.ts \
    i18n/respeqt_es.ts \
    i18n/respeqt_pl.ts \
    i18n/respeqt_ru.ts \
    i18n/respeqt_sk.ts \
    i18n/respeqt_tr.ts

RC_FILE = RespeQt.rc \

DISTFILES += \
    test.xml




