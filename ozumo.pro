#-------------------------------------------------
#
# Project created by QtCreator 2011-01-24T09:22:49
#
#-------------------------------------------------

QT      += core gui sql

win32 {
CONFIG  -= debug_and_release
CONFIG  += release

QMAKE_LFLAGS += -static
}

TARGET = ozumo
TEMPLATE = app

DESTDIR = release

MOC_DIR = intermediate_files
OBJECTS_DIR = intermediate_files
RCC_DIR = intermediate_files
UI_DIR = intermediate_files
UI_HEADERS_DIR = intermediate_files
UI_SOURCES_DIR = intermediate_files

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS += mainwindow.h

FORMS   += mainwindow.ui
