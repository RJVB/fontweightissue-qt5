QT += widgets core-private gui-private
CONFIG += release c++11 rpath
QMAKE_CXXFLAGS_RELEASE -= -pipe -O2
QMAKE_CXXFLAGS_RELEASE += -g -O3 -march=native

HEADERS       = dialog.h timing.c timing.h \
                kwidgetsaddons/fonthelpers_p.h \
                kwidgetsaddons/kfontchooser.h \
                kwidgetsaddons/kfontchooserdialog.h \
                kwidgetsaddons/kfontrequester.h
SOURCES       = dialog.cpp \
                main.cpp \
                kwidgetsaddons/kfontchooser.cpp \
                kwidgetsaddons/kfontchooserdialog.cpp \
                kwidgetsaddons/kfontrequester.cpp \
                kwidgetsaddons/fonthelpers.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/fontweightissue
INSTALLS += target

wince50standard-x86-msvc2005: LIBS += libcmt.lib corelibc.lib ole32.lib oleaut32.lib uuid.lib commctrl.lib coredll.lib winsock.lib ws2.lib
