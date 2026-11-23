QT += core gui widgets sql

CONFIG += c++11
TEMPLATE = app
TARGET = Lab_3

# Windows + MinGW: 保证源码中的中文字面量按 UTF-8 编译/执行
win32-g++ {
    QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    dbmanager.cpp \
    loginpage.cpp \
    registerdialog.cpp \
    welcomepage.cpp \
    patientpage.cpp \
    patienteditdialog.cpp \
    doctorpage.cpp \
    doctoreditdialog.cpp \
    departmentpage.cpp \
    departmenteditdialog.cpp \
    historypage.cpp \
    delegates/comboboxdelegate.cpp

HEADERS += \
    mainwindow.h \
    config.h \
    dbmanager.h \
    loginpage.h \
    registerdialog.h \
    welcomepage.h \
    patientpage.h \
    patienteditdialog.h \
    doctorpage.h \
    doctoreditdialog.h \
    departmentpage.h \
    departmenteditdialog.h \
    historypage.h \
    delegates/comboboxdelegate.h

RESOURCES += resources.qrc

# 部署时把 dbconfig.ini 放在可执行文件同目录
