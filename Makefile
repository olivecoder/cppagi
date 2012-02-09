# $Id: Makefile 1093 2009-09-15 20:04:42Z robert $
# Robert Almeida - 25/6/2002
# updating history:
# 2006.02.10 ROBERT split cppagi and telemsg targets

# compiler & linker flags
CP	 = cp
LDCONFIG = ldconfig
OPT      = -Wall -O3 -s -W -fexceptions -fPIC
DEBUG    = -Wall -O0 -fexceptions -ggdb -DDEBUG
CFLAGS   = $(INCPATH) $(LIBSPATH) $(OPT)
CXXFLAGS += $(CFLAGS)
LFLAGS   = 

# auto targgets & prerequisites
CSOURCE   = $(wildcard *.c) 
CPPSOURCE = $(wildcard *.cpp)

# automatic variables
COBJS    = $(patsubst %.c, %.o, $(CSOURCE))
CPPOBJS  = $(patsubst %.cpp, %.o, $(CPPSOURCE))
SOURCE   = $(CSOURCE) $(CPPSOURCE) 
OBJS     = $(COBJS) $(CPPOBJS) 

# project dependent
DLLS     = libcppagi.so
LIBS	 =

# project dependent rules
cppagi.o: 	cppagi.h

# general rules
# $< 
all: $(OBJS) $(APPS) $(DLLS)

#%.o: %.cpp %.h
#	$(CXX) $< $(CXXFLAGS) -c -o $@ 

#%.o: %.c %.h
#	$(CC) $< $(CFLAGS) -c -o $@ 

#$.so: %.o
#	$(CXX) $^ -shared $(CXXFLAGS) $(LFLAGS) -o $@ $(LIBS)

#$(APPS): $(OBJS)
#	$(CXX) $(CXXFLAGS) $(LFLAGS) -o $@ $^ $(LIBS) 
#	$(CXX) $(CXXFLAGS) $(LFLAGS) -o $@ $^ $(LIBS) 

libcppagi.so: cppagi.o asteriskconf.o
	$(CXX) $^ -shared $(CXXFLAGS) $(LFLAGS) -o $@ 

clean:
	$(RM) $(APPS)
	$(RM) $(OBJS)
	$(RM) $(DLLS)
	$(RM) *~

build:
	$(MAKE) clean
	$(MAKE) all

install:
	$(MAKE) libcppagi.so
	$(CP) libcppagi.so /usr/lib
	$(CP) cppagi.h /usr/include


