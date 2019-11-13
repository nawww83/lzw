# Консольная утилита для сжатия/распаковки радарных данных

include(../src.pri)

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp



CONFIG(debug, debug|release) {
    # Подключаем debug-версии библиотек для разных платформ
    win32: LIBS += -L../lzwhash/debug/ -llzwhashd1
    unix: LIBS += -L../lzwhash/ -L. -llzwhashd -Wl,-rpath,lib -Wl,-rpath,.
} else {
    # Подключаем release-версии библиотек для разных платформ
    win32: LIBS += -L../lzwhash/release/ -llzwhash1
    unix: LIBS += -L../lzwhash/ -L. -llzwhash -Wl,-rpath,lib -Wl,-rpath,.
}


INCLUDEPATH += ../lzwhash/

#unix:{
#    QMAKE_LFLAGS += '-Wl,-rpath,\'.\''
#}
