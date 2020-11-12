SHELL = /bin/bash

INCLUDES = lists.h dispatcher.h main.h appFrame.h displayTab.h diagTab.h \
	recordingsTab.h ecsDataTab.h settingsBlock.h settingsTab.h maxon.h \
	definitions.h libftp.h framebuf.h plotting.h plotsTab.h
SOURCES = main.cpp appFrame.cpp displayTab.cpp diagTab.cpp \
	recordingsTab.cpp ecsDataTab.cpp settingsBlock.cpp settingsTab.cpp \
	maxon.cpp libftp.cpp framebuf.cpp plotting.cpp plotsTab.cpp
OBJECTS = main.o appFrame.o displayTab.o diagTab.o recordingsTab.o \
	ecsDataTab.o settingsBlock.o settingsTab.o maxon.o libftp.o framebuf.o \
	plotting.o plotsTab.o

CXX = g++
#CXX = g++4.7.0
INC64 = /usr/include
#INC64 = /sw/include
LIB64 = /usr/lib64
#LIB64 = /sw/lib
M32 =
#M32 = -m32
DEBUG = -g
#DEBUG =
OPT =
#OPT = -O3

ADROOT = /home/avirisng/NGDCS/AlphaData/adframeserver
ADLIB = ${ADROOT}/api/linux
ADCLINK = ${ADLIB}/adclink/dist/ReleaseGen3/GNU-Linux-x86
ADCOMMON = ${ADLIB}/adcommon/dist/ReleaseGen3/GNU-Linux-x86
ADFRAMESERVER = ${ADLIB}/adframeserver/dist/Release/GNU-Linux-x86
ADFRAMECLIENT = ${ADLIB}/adframeclient/dist/Release/GNU-Linux-x86

INCLUDEDIRFLAGS = \
	-I$(INC64) \
	-I$(INC64)/atk-1.0 \
	-I$(INC64)/atkmm-1.6 \
	-I$(INC64)/cairo \
	-I$(INC64)/cairomm-1.0 \
	-I$(INC64)/gdkmm-2.4 \
	-I$(INC64)/giomm-2.4 \
	-I$(INC64)/glib-2.0 \
	-I$(INC64)/glibmm-2.4 \
	-I$(INC64)/glibmm-2.4/include \
	-I$(INC64)/gtk-2.0 \
	-I$(INC64)/gtkmm-2.4 \
	-I$(INC64)/pango-1.0 \
	-I$(INC64)/pangomm-1.4 \
	-I$(INC64)/sigc++-2.0 \
	-I$(LIB64)/freetype219/include/freetype2 \
	-I$(LIB64)/gdkmm-2.4/include \
	-I$(LIB64)/glib-2.0/include \
	-I$(LIB64)/glibmm-2.4/include \
	-I$(LIB64)/gtk-2.0/include \
	-I$(LIB64)/gtkmm-2.4/include \
	-I$(LIB64)/pangomm-1.4/include \
	-I$(LIB64)/sigc++-2.0/include \
	-I$(INC64)/freetype2 \
	-I$(INC64)/gdk-pixbuf-2.0 \
	-I$(INC64)/tomcrypt \
	-I/usr/local/include \
	-I${ADROOT}/api/include \
	-I${ADROOT}/deps/linux/include \
	-I${ADROOT}/deps/common/include
CCFLAGS = $(M32) $(DEBUG) $(OPT) -Wall $(INCLUDEDIRFLAGS) -D_LINUX -pthread \
	    -D_EXCLUDE_ADMXRC_GEN2
LDFLAGS = -L$(LIB64) -lgtkmm-2.4 -lsigc-2.0 -lglibmm-2.4 -latkmm-1.6 \
	    -lgdkmm-2.4 -lpangomm-1.4 -lcairomm-1.0 -lgdk-x11-2.0 \
	    -lgtk-x11-2.0 -lipc -ltomcrypt ${ADCLINK}/libadclink_gen3.a \
	    ${ADCOMMON}/libadcommon_gen3.a ${ADFRAMESERVER}/libadframeserver.a \
	    ${ADFRAMECLIENT}/libadframeclient.a -lpthread -ladmxrc3 \
	    -lgthread-2.0 /home/avirisng/NGDCS/AlphaData/adframeserver/apps/linux/jplclink_console/build/Release/GNU-Linux-x86/_ext/1249851811/cameracard_jpl_adb3.o
LINT_HOME = /Users/alan/Lint

ngdcs: $(OBJECTS)
	g++ $(M32) $(DEBUG) $(OBJECTS) -o ngdcs $(LDFLAGS)
	@if [ -e ~/.ngislinux ] ; then \
	    mv ~/.ngislinux ~/.ngdcs ; \
	fi

$(OBJECTS): $(INCLUDES)

main.o: main.cpp
	$(CXX) $(CCFLAGS) -c main.cpp 

appFrame.o: appFrame.cpp
	$(CXX) $(CCFLAGS) -c appFrame.cpp 

displayTab.o: displayTab.cpp
	$(CXX) $(CCFLAGS) -c displayTab.cpp 

diagTab.o: diagTab.cpp
	$(CXX) $(CCFLAGS) -c diagTab.cpp 

plotsTab.o: plotsTab.cpp
	$(CXX) $(CCFLAGS) -c plotsTab.cpp 

recordingsTab.o: recordingsTab.cpp
	$(CXX) $(CCFLAGS) -c recordingsTab.cpp 

ecsDataTab.o: ecsDataTab.cpp
	$(CXX) $(CCFLAGS) -c ecsDataTab.cpp 

settingsBlock.o: settingsBlock.cpp
	$(CXX) $(CCFLAGS) -c settingsBlock.cpp 

settingsTab.o: settingsTab.cpp
	$(CXX) $(CCFLAGS) -c settingsTab.cpp 

maxon.o: maxon.cpp
	$(CXX) $(CCFLAGS) -c maxon.cpp 

libftp.o: libftp.cpp
	$(CXX) $(CCFLAGS) -c libftp.cpp 

framebuf.o: framebuf.cpp
	$(CXX) $(CCFLAGS) -c framebuf.cpp 

plotting.o: plotting.cpp
	$(CXX) $(CCFLAGS) -c plotting.cpp 

clean:
	rm -f *.o ngdcs

BITFILE = xrm-clink-mfb-adb3-db-adpexrc6t-6vlx240t.bit
BITFILELOCALDIR = xrm-clink-mfb-adb3
BITFILEINSTALLDIR = /usr/local/lib/xrm-clink-mfb-adb3

install:
	cp -p ngdcs /usr/local/bin
	@if [ ! -e $(BITFILEINSTALLDIR) ] ; then \
	    mkdir $(BITFILEINSTALLDIR) ; \
	    cp $(BITFILELOCALDIR)/$(BITFILE) $(BITFILEINSTALLDIR) ; \
	elif [ ! -e $(BITFILEINSTALLDIR)/$(BITFILE) ] ; then \
	    cp $(BITFILELOCALDIR)/$(BITFILE) $(BITFILEINSTALLDIR) ; \
	elif ./confirmcp ; then \
	    cp $(BITFILELOCALDIR)/$(BITFILE) $(BITFILEINSTALLDIR) ; \
	fi

lint:
	$(LINT_HOME)/flint -i $(LINT_HOME) $(INCLUDEDIRFLAGS) -d_lint=1 \
	    +libh\(definitions.h\) std.lnt ngdcs.lnt $(SOURCES)
