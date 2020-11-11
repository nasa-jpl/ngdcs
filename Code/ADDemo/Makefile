CC=gcc
CCC=g++
CXX=g++

# Object Files
OBJECTFILES= ngis.o
INCLUDES = ../framebuf.h
ADROOT = /home/avirisng/NGISLinux/AlphaData/Demo

INCLUDEFLAGS = -I${ADROOT}/api/include -I${ADROOT}/deps/linux/include -I${ADROOT}/deps/common/include
OTHERCFLAGS = -g -D_EXCLUDE_ADMXRC_GEN2 -D_LINUX -pthread

# C Compiler Flags
CFLAGS=$(INCLUDEFLAGS) $(OTHERCFLAGS)

# CC Compiler Flags
CCFLAGS=$(INCLUDEFLAGS) $(OTHERCFLAGS)
CXXFLAGS=$(INCLUDEFLAGS) $(OTHERCFLAGS)

# Link Libraries and Options
LDLIBSOPTIONS=${ADROOT}/api/linux/adclink/dist/ReleaseGen3/GNU-Linux-x86/libadclink_gen3.a ${ADROOT}/api/linux/adcommon/dist/ReleaseGen3/GNU-Linux-x86/libadcommon_gen3.a -lpthread -ladmxrc3 -lX11

# Build Targets
build: ngis

${OBJECTFILES}: ${INCLUDES}

ngis: ${OBJECTFILES}
	${LINK.cc} -o ngis ${OBJECTFILES} ../framebuf.o ${LDLIBSOPTIONS} 

ngis.o: ngis.cpp

clean:
	/bin/rm -f *.o
	/bin/rm -f ngis
