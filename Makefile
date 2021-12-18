# Minimal Makefile

# build shared library with -fPIC, -shared
CFLAGS   = -g # -O3 # -fPIC  # CXXFLAGS for .cpp
LDFLAGS  = # -L../hello # -shared
LDLIBS   = -lpthread
CPPFLAGS = -MMD -MP # -I../hello
#CC      = $(CXX)  # link with CXX for .cpp

# target name is basename of one of the source files
example : $(patsubst %.c,%.o,$(wildcard *.c))  # .cpp
-include *.d
clean : ; -rm -fr *.o *.d
.PHONY : clean
