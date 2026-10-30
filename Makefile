# World Conquest - Nintendo 3DS
# Toolchain: devkitARM / libctru / citro3D

.SUFFIXES:

TARGET      := WorldConquest
BUILD       := build
SOURCES     := source
INCLUDES    := include

APP_TITLE       := World Conquest
APP_DESCRIPTION := Build your own nation and conquer a procedural world
APP_AUTHOR      := World Conquest Team

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS   := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH)
CXXFLAGS := $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS  = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections

LIBS    := -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB)

include $(DEVKITARM)/3ds_rules
