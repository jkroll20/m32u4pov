#
#             LUFA Library
#     Copyright (C) Dean Camera, 2012.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

MCU          = atmega32u4
ARCH         = AVR8
BOARD        = USBKEY
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = airpov
SRC          = $(TARGET).cpp 
#Descriptors.c $(LUFA_SRC_USB)
LUFA_PATH    = $(HOME)/src/LUFA-120730/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/
CPP_FLAGS    = -std=c++0x -Wl,-u,vfprintf -lprintf_min
LD_FLAGS     = -Wl,-u,vfprintf -lprintf_min

# Default target
all:    font.h sine.h

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk

SERIAL=/dev/ttyACM0
upload: all
	#-./midireset.py
	while ! stat $(SERIAL) >/dev/null ; do sleep .5; done
	sleep .25
	avrdude -p m32u4 -c avr109 -P $(SERIAL) -U flash:w:$(TARGET).hex

font.h:	data/font.c data/mkfont.c
	gcc -std=c99 data/mkfont.c -o data/mkfont
	data/mkfont > font.h

sine.h:	data/mksine.c
	gcc -std=c99 data/mksine.c -lm -o data/mksine
	data/mksine > sine.h
