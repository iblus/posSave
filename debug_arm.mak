#Generated by VisualGDB (http://visualgdb.com)
#DO NOT EDIT THIS FILE MANUALLY UNLESS YOU ABSOLUTELY NEED TO
#USE VISUALGDB PROJECT PROPERTIES DIALOG INSTEAD

BINARYDIR := Debug_arm

#Toolchain
CC := /home/leador/ti/arm-2011.03/bin/arm-none-linux-gnueabi-gcc
CXX := /home/leador/ti/arm-2011.03/bin/arm-none-linux-gnueabi-g++
LD := $(CXX)
AR := /home/leador/ti/arm-2011.03/bin/arm-none-linux-gnueabi-ar
OBJCOPY := /home/leador/ti/arm-2011.03/bin/arm-none-linux-gnueabi-objcopy

#Additional flags
PREPROCESSOR_MACROS := DEBUG=1
INCLUDE_DIRS :=
LIBRARY_DIRS :=
LIBRARY_NAMES := pthread
ADDITIONAL_LINKER_INPUTS := 
MACOS_FRAMEWORKS := 
LINUX_PACKAGES := 

CFLAGS := -ggdb -ffunction-sections -O0
CXXFLAGS := -ggdb -ffunction-sections -O0
ASFLAGS := 
LDFLAGS := -Wl,-gc-sections
COMMONFLAGS := 
LINKER_SCRIPT := 

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

#Additional options detected from testing the toolchain
IS_LINUX_PROJECT := 1
