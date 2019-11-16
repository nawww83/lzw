# GUI для примера использования библиотеки сжатия

include(../src.pri)

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = guilzw

CONFIG(debug, debug|release) {
    # Подключаем debug-версии библиотек для разных платформ
    win32: LIBS += -L../lzwhash/debug/ -llzwhashd1
    unix: LIBS += -L../lzwhash/ -L. -llzwhashd -Wl,-rpath,lib -Wl,-rpath,.
} else {
    # Подключаем release-версии библиотек для разных платформ
    win32: LIBS += -L../lzwhash/release/ -llzwhash1
    unix: LIBS += -L../lzwhash/ -L. -llzwhash -Wl,-rpath,lib -Wl,-rpath,.
}

SOURCES += main.cpp\
    lzw_thread.cpp \
        widget.cpp \

HEADERS  += widget.h \
    lzw_thread.h

INCLUDEPATH +=  ../lzwhash/

FORMS    += widget.ui

RESOURCES += \
    line.qrc
