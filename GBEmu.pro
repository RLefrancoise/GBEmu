#-------------------------------------------------
#
# Project created by QtCreator 2012-08-24T16:32:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GBEmu
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    gb/cpu.cpp \
    gb/mmu.cpp

HEADERS  += mainwindow.h \
    gb/cpu.h \
    gb/mmu.h

FORMS    += mainwindow.ui

INCLUDEPATH +=  C:/Users/Renaud/Documents/programmation/C++/CMake/MMO/SRC/Lib/Utils  \
                C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/include    \
                C:/Users/Renaud/Documents/programmation/C++/CMake/Plateau/thirdParty/boost/include

LIBS += C:/Users/Renaud/Documents/programmation/C++/CMake/MMO/lib/Debug/UTILS_lib.lib   \
        C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/lib/sfml-audio-d.lib    \
        C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/lib/sfml-graphics-d.lib \
        C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/lib/sfml-main-d.lib        \
        C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/lib/sfml-network-d.lib      \
        C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/lib/sfml-system-d.lib    \
        C:/Users/Renaud/Documents/programmation/C++/Qt/VLE/VLE/3rdParty/SFML/lib/sfml-window-d.lib
