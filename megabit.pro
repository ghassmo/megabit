QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = megabit
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH = include/megabit/

QMAKE_CXXFLAGS += -fpermissive -Wno-ignored-qualifiers -Wno-deprecated-declarations -static

SOURCES += src/utils.cpp \
           src/bitcoin_interface.cpp \
           src/createwalletintroduction.cpp \
           src/createwalletgenerate.cpp \
           src/createwalletconfirm.cpp \
           src/createwalletwizard.cpp \
           src/block_height_thread.cpp \
           src/address_monitor_thread.cpp \
           src/send_payment_thread.cpp \
           src/settings.cpp \
           src/megabit.cpp \
           src/main.cpp

HEADERS += include/megabit/constants.hpp \
           include/megabit/utils.hpp \
           include/megabit/bitcoin_interface.hpp \
           include/megabit/createwalletintroduction.hpp \
           include/megabit/createwalletgenerate.hpp \
           include/megabit/createwalletconfirm.hpp \
           include/megabit/createwalletwizard.hpp \
           include/megabit/block_height_thread.hpp \
           include/megabit/address_monitor_thread.hpp \
           include/megabit/send_payment_thread.hpp \
           include/megabit/settings.hpp \
           include/megabit/constants.hpp \
           include/megabit/megabit.hpp

RESOURCES += resources/megabit.qrc

FORMS += forms/megabit.ui

#DESTDIR=dist

OBJECTS_DIR=build

MOC_DIR=build

CONFIG += c++11 link_pkgconfig debug static

macx{
        QT_CONFIG -= no-pkg-config
}
PKGCONFIG += libbitcoin-client

LIBS += -lz
