#######################################################################
#  makefile for UNIX GlobalCall demo programs
# 
#  IMPORTANT:
#       This makefile must be configured for your particular
#       operating system.  See the "Operating System section"
#       below.              
#
#       Modify this makefile to match your specific configuration.
#
########################################################################


########################################################################
# Operating System section begin

# Linux
#
OS_CFLAGS     = -Wall
SYSLIBS       =

# Solaris
#
#OS_CFLAGS     = 
#SYSLIBS       =

# UnixWare 7
#
#OS_CFLAGS     = 
#SYSLIBS       =

# Operating System section end
########################################################################

# Handle both SR5.1 and SR6/later
ifdef INTEL_DIALOGIC_INC
CFLAGS        = -m32 -g $(OS_CFLAGS) -I$(INTEL_DIALOGIC_INC) -Dlint
else
CFLAGS        = -g $(OS_CFLAGS) -Dlint
endif

# Intel Dialogic libraries to use
# Handle both SR5.1 and SR6/later
ifdef INTEL_DIALOGIC_LIB
LIBS      = -m32 -L$(INTEL_DIALOGIC_LIB) -lgc
else
LIBS      =  -lgc
endif

OBJS_CALLFLOW       = callflow.o



all: callflow

callflow: $(OBJS_CALLFLOW)
	$(CC) $(OBJS_CALLFLOW) $(LIBS) $(SYSLIBS) -o $@

clean:
	rm -f *.o callflow

