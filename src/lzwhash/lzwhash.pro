# Библиотека сжатия/распаковки радарных данных

#include(../src.pri)

TEMPLATE = lib

VERSION = 1.0.0

unix: {
        CONFIG (debug, debug|release) {
                # Такое название имеет debug-версия библиотеки
                TARGET = lzwhashd
        } else {
                # А такое release-версия
                TARGET = lzwhash
        }
} else {
        TARGET = $$qtLibraryTarget(lzwhash)
}

CONFIG += debug_and_release build_all

# Первый параметр необходим для сборки #библиотеки в linux (qmake, make all),
# второй для сборки под остальными ОС.
#CONFIG += debug_and_release build_all
# Указываем папки для объектных файлов. Для unix-подобных ОС это критично.
# Если этого не сделать, то будет собираться только release версия библиотеки,
# либо только отладочная. Связано это с тем, что файлы будут замещать друг друга.
CONFIG (debug, debug|release) {
        OBJECTS_DIR = build/debug
} else {
        OBJECTS_DIR = build/release
}


HEADERS += \
    lzw_global.h \
    lzwdefs.h \
    hash_p.h \
    lzw_p.h \
    lzw.h

SOURCES += hash.cpp\
        lzw.cpp

DEFINES += EXPORTING_LZW
