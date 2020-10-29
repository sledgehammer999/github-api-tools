TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

TARGET = mass_close_old_issues

DEFINES += BOOST_BEAST_USE_STD_STRING_VIEW

QMAKE_CXXFLAGS_RELEASE += $$quote(-isystemG:/QBITTORRENT/boost_1_74_0)
QMAKE_CXXFLAGS_RELEASE += $$quote(-isystemG:/QBITTORRENT/install_mingw/base/include)

LIBS += $$quote(-LG:/QBITTORRENT/boost_1_74_0/stage/lib)
LIBS += $$quote(-LG:/QBITTORRENT/install_mingw/base/lib)

LIBS += libboost_program_options-mgw9-mt-s-x64-1_74
LIBS += -lssl -lcrypto -lz -lgdi32 -luser32 -lws2_32 -ladvapi32 -lcrypt32

HEADERS += issuegatherer.h \
           issueupdater.h \
           labelcreator.h \
           labelgatherer.h \
           postdownloader.h \
           programoptions.h

SOURCES += main.cpp \
           issuegatherer.cpp \
           issueupdater.cpp \
           labelcreator.cpp \
           labelgatherer.cpp \
           postdownloader.cpp \
           programoptions.cpp
