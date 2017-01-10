#=======================================================================================
#
#     Filename: Makefile
#  Description: 
#
#        Usage: make              (generate target, equal to make debug )
#               make release      (generate release target              )
#               make debug        (generate debug target                )
#               make clean        (remove objects, target, prerequisits )
#
#      Version: 1.0
#      Created: 
#     Revision: ---
#
#       Author: 
#      Company: 
#        Email: 
#
#        Notes: C   extension   :  c 
#               C++ extensions  :  cc cpp C 
#               C and C++ sources can be mixed.
#               Prerequisites are generated automatically; makedepend is not
#               needed (see documentation for GNU make Version 3.80, July 2002,
#               section 4.13). The utility sed is used.
#               
#============================================== makefile template version 1.6 ==========

# ----------- custom setting ----------------------------------------------------------
PROJECT_TOPDIR  = ../../..
include $(PROJECT_TOPDIR)/config.mk

# ------------  name of the target  ------------------------------------------------
TARGET          = libpsmux.a

# ------------  list of all source files  ----------------------------------------------
SOURCES         = psmux.c psmuxstream.c psutil.c
# ------------  compiler  --------------------------------------------------------------
CC              = $(TOOLCHAIN)gcc
CXX             = $(TOOLCHAIN)g++
AR              = $(TOOLCHAIN)ar

# ------------  compiler flags  --------------------------------------------------------
ifeq ($(MAKECMDGOALS), debug)   
    CFLAGS      = -Wall -O0 -g
else
    CFLAGS      = -O3
endif   
ifeq ($(NXP_CHIPSET), ASC8850)    
	CFLAGS += -DASC8850_M2
else     
	CFLAGS += -DASC8850_M3
endif
CFLAGS += -DSVN=\"$(shell svnversion -n ../..)\" -DPRODUCT_MODEL=\"MK_HD_CAM\"
CFLAGS += -D__ASM_ARCH_PLATFORM_MOZART_H__
CFLAGS += -DNXP_CHIPSET_ASC8850
CFLAGS += -DNXP_SDK_VERSION=$(NXP_SDK_VERSION)

# ------------  linker-Flags  ----------------------------------------------------------
LFLAGS          = -g

# ------------  additional system include directories  ---------------------------------
GLOBAL_INC_DIR  = $(SDK_INCLUDE)

# ------------  private include directories  -------------------------------------------
LOCAL_INC_DIR   = glib

# ------------  system libraries  (e.g. -lm )  -----------------------------------------
SYS_LIBS        = -lm -ldl -lrt

# ------------  additional system library directories  ---------------------------------
GLOBAL_LIB_DIR  = 

# ------------  additional system libraries  -------------------------------------------
GLOBAL_LIBS     = 

# ------------  private library directories  -------------------------------------------
LOCAL_LIB_DIR   = glib

# ------------  private libraries  (e.g. libxyz.a )  -----------------------------------
LOCAL_LIBS      = 

# ------------  archive generation -----------------------------------------------------
TARBALL_EXCLUDE = *.{o,gz,zip}
ZIP_EXCLUDE     = *.{o,gz,zip}

# ------------  run target out of this Makefile  (yes/no)  -------------------------
# ------------  cmd line parameters for this target  -------------------------------
EXE_START       = no
EXE_CMDLINE     = 

#=======================================================================================
# The following statements usually need not to be changed
#=======================================================================================

C_SOURCES       = $(filter     %.c, $(SOURCES))
CPP_SOURCES     = $(filter-out %.c, $(SOURCES))
ALL_INC_DIR     = $(addprefix -I, $(LOCAL_INC_DIR) $(GLOBAL_INC_DIR))
ALL_LIB_DIR     = $(addprefix -L, $(LOCAL_LIB_DIR) $(GLOBAL_LIB_DIR))
ALL_CFLAGS      = $(CFLAGS) $(ALL_INC_DIR)
ALL_LFLAGS      = $(LFLAGS) $(ALL_LIB_DIR)
BASENAMES       = $(basename $(SOURCES))

# ------------  generate the names of the object files  --------------------------------
OBJECTS         = $(addsuffix .o,$(BASENAMES))

# ------------  generate the names of the hidden prerequisite files  -------------------
PREREQUISITES   = $(addprefix .,$(addsuffix .d,$(BASENAMES)))

# ------------  make the target  ---------------------------------------------------
$(TARGET):  	$(OBJECTS)
				@$(AR) -r $@ $^

release:        $(TARGET)

debug:          $(TARGET)

# ------------  include the automatically generated prerequisites  ---------------------
# ------------  if target is not clean, tarball or zip             ---------------------
ifneq ($(MAKECMDGOALS),clean)
include         $(PREREQUISITES)
endif

# ------------  make the objects  ------------------------------------------------------
%.o:			%.c
				$(CC)  -c $(ALL_CFLAGS) $< 

%.o:			%.cc
				$(CXX) -c $(ALL_CFLAGS) $< 

%.o:			%.cpp
				$(CXX) -c $(ALL_CFLAGS) $< 

%.o:			%.C
				$(CXX) -c $(ALL_CFLAGS) $< 

# ------------  make the prerequisites  ------------------------------------------------
#
.%.d:           %.c
				@$(make-prerequisite-c)

.%.d:			%.cc
				@$(make-prerequisite-cplusplus)

.%.d:			%.cpp
				@$(make-prerequisite-cplusplus)

.%.d:			%.C
				@$(make-prerequisite-cplusplus)

#  canned command sequences
#  echoing of the sed command is suppressed by the leading @

define	make-prerequisite-c
				$(CC)   -M $(ALL_CFLAGS) $< > $@.$$$$;            \
				sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' < $@.$$$$ > $@; \
				rm -f $@.$$$$; 
endef

define	make-prerequisite-cplusplus
				$(CXX)  -M $(ALL_CFLAGS) $< > $@.$$$$;            \
				sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' < $@.$$$$ > $@; \
				rm -f $@.$$$$; 
endef

# ------------  remove generated files  ------------------------------------------------
# ------------  remove hidden backup files  --------------------------------------------
clean:
				rm  --force  $(TARGET) $(OBJECTS) $(PREREQUISITES) *~

# ======================================================================================
# vim: set tabstop=2: set shiftwidth=2: 
# DO NOT DELETE 
