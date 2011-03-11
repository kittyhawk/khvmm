######################################################################
##
## Copyright (C) 2008, Udo Steinberg, IBM Corporation
##
## File path:     Makefile
## Description:
##
## All rights reserved
##
######################################################################

-include Makeconf

CROSS		?= powerpc-linux-
CC		:= $(CROSS)gcc
LD		:= $(CROSS)ld
RM		:= rm -f

GCCINSTALLDIR	?= $(dir $(shell $(CC) -print-libgcc-file-name))
LIBGCCINC	?= $(GCCINSTALLDIR)include
LIBGCCDIR	?= $(GCCINSTALLDIR)

TARGET		:= khvmm
USR_DIR		?= ${HOME}/src/pistachio-hg/build/powerpc/user/
SRC_DIR		:= ../src
INC_DIR		:= ../src $(USR_DIR)/include $(GCCINSTALLDIR)include
LIB_DIR		:= $(USR_DIR)/lib $(LIBGCCDIR)

# Sources
LIB		:= l4 io gcc
SRC		:= khvmm.ld $(wildcard $(SRC_DIR)/*.S) $(wildcard $(SRC_DIR)/*.cc)
OBJ		:= $(notdir		\
		   $(patsubst %.ld,%.o,	\
		   $(patsubst %.S,%.o,	\
                   $(patsubst %.cc,%.o, $(SRC)))))
DEP		:= $(patsubst %.o,%.d, $(OBJ))

# Feature check
check = $(shell if $(CC) $(1) -S -xc++ /dev/null -o /dev/null >/dev/null 2>&1; then echo "$(1)"; fi)

# Preprocessor options
DEFINES		:=
VPATH		:= $(SRC_DIR)
PFLAGS		:= $(addprefix -D,$(DEFINES)) $(addprefix -I,$(INC_DIR))

# Optimization options
AFLAGS		:= -mno-toc
DFLAGS		:= -MP -MMD -pipe
OFLAGS		:= -Os -g

# Language options
FFLAGS		:= -fdata-sections -ffunction-sections -fomit-frame-pointer -freg-struct-return -freorder-blocks -funit-at-a-time
FFLAGS		+= -fno-exceptions -fno-rtti -fno-builtin
FFLAGS		+= $(call check,-fno-stack-protector)
FFLAGS		+= $(call check,-fvisibility-inlines-hidden)

# Warning options
WFLAGS          := -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wformat=2 -Wmissing-format-attribute -Wmissing-noreturn -Wpointer-arith -Wredundant-decls -Wshadow -Wwrite-strings
WFLAGS		+= -Wabi -Wctor-dtor-privacy -Wno-non-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wpacked
# -Wold-style-cast -Waggregate-return

MFLAGS		:= -mabi=altivec

# Compiler flags
SFLAGS		:= $(PFLAGS) $(DFLAGS)
CFLAGS		:= $(PFLAGS) $(DFLAGS) $(OFLAGS) $(AFLAGS) $(FFLAGS) $(WFLAGS) $(MFLAGS)

# Linker flags
LFLAGS		:= --gc-sections --warn-common -static -N $(addprefix -L,$(LIB_DIR)) -T

# Targets
$(TARGET):	$(OBJ)
		$(LD) $(LFLAGS) $^ $(addprefix -l,$(LIB)) -o $@

.PHONY:		clean
.PHONY:		cleanall

clean:
		$(RM) $(OBJ) $(TARGET)

cleanall:	clean
		$(RM) $(DEP)

# Rules
%.o:		%.cc $(MAKEFILE_LIST)
		$(CC) $(CFLAGS) -c $< -o $@

%.o:		%.S $(MAKEFILE_LIST)
		$(CC) $(SFLAGS) -c $< -o $@

%.o:		%.ld $(MAKEFILE_LIST)
		$(CC) $(SFLAGS) -xc -undef -E -P $< -o $@

# Dependencies
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleanall)
-include	$(DEP)
endif
endif
