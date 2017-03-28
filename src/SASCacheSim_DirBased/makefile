##############################################################
#
# Here are some things you might want to configure
#
##############################################################


CC  = g++
CXX = g++
AS  = g++ -c

-include $(PIN_HOME)/makefile.gnu.config 

##############################################################
#
# Tools sets
#
##############################################################
OPT = -g -O0
#OPT = -03

#CXXFLAGS := -ansi -Wall ${PIN_CXXFLAGS} -MMD -DTARGET_LINUX -D__cplusplus -D_M_IX86 ${OPT} -DTARGET_IA32 -I${PIN_HOME}/InstLib
CXXFLAGS := -ansi -Wall ${PIN_CXXFLAGS} -MMD -D__DEBUG__ -DTARGET_LINUX -D__cplusplus -D_M_IX86 ${OPT} -DTARGET_IA32 -I${PIN_HOME}/InstLib

LDLIBS   := ${PIN_LPATHS} ${PIN_LIBS}
LDFLAGS  := ${PIN_LDFLAGS} 

SOURCES := sascache.cpp callbacks.cpp
DEPENDS := cache.H callbacks.H profile.H coherenece.H

OBJS := ${SOURCES:.cpp=.o}

all: SASCache

SASCache: ${OBJS}
	${CXX} ${LDFLAGS} ${OBJS} ${LDLIBS} -o $@

tools: $(TOOLS)
test: $(TEST_TOOLS:%=%.test)
tests-sanity: $(SANITY_TOOLS:%=%.test)

## cleaning
clean:
	-rm -f *.o $(OBJS) SASCache *.out *.tested *.failed *.d *makefile.copy *.exp *.lib *~

-include *.d

