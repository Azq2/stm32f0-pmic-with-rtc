PROJECT = stm32f0-pmic-with-rtc
BUILD_DIR = bin

CFLAGS += -fno-builtin-printf
CXXFLAGS += -fno-builtin-printf

########################################################################

# app
CXXFILES += src/App.cpp
CXXFILES += src/AnalogMon.cpp
CXXFILES += src/Exti.cpp
CXXFILES += src/Buzzer.cpp
CXXFILES += src/Button.cpp
CXXFILES += src/Loop.cpp
CXXFILES += src/Task.cpp
CXXFILES += src/RTC.cpp
CXXFILES += src/I2CSlave.cpp
CXXFILES += src/main.cpp
CXXFILES += src/utils.cpp

# delegate
INCLUDES += -Ilib/delegate/include

# printf
CFILES += lib/printf/src/printf/printf.c
INCLUDES += -Ilib/printf/src
CFLAGS += -DPRINTF_INCLUDE_CONFIG_H=0 -DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES_HARD=1 -DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES=1
CFLAGS += -DPRINTF_SUPPORT_LONG_LONG=0 -DPRINTF_SUPPORT_DECIMAL_SPECIFIERS=0 -DPRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS=0
CFLAGS += -DPRINTF_SUPPORT_WRITEBACK_SPECIFIER=0

########################################################################

DEVICE=stm32f030x4
OOCD_FILE = board/stm32f0discovery.cfg

INCLUDES += -Isrc
INCLUDES += $(patsubst %,-I%, .)
OPENCM3_DIR=lib/libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

ifeq ($(BMP_PORT),)
	BMP_PORT_CANDIDATES := $(wildcard /dev/serial/by-id/usb-*Black_Magic_Probe_*-if00)
	ifeq ($(words $(BMP_PORT_CANDIDATES)),1)
		BMP_PORT := $(BMP_PORT_CANDIDATES)
	else
		BMP_PORT = $(error Black Magic Probe gdb serial port not found, please provide the device name via the BMP_PORT variable parameter$(if $(BMP_PORT_CANDIDATES), (found $(BMP_PORT_CANDIDATES))))
	endif
endif

ifeq ($(SERIAL_PORT),)
	SERIAL_PORT_CANDIDATES := $(wildcard /dev/serial/by-id/usb-*Black_Magic_Probe_*-if02)
	ifeq ($(words $(SERIAL_PORT_CANDIDATES)),1)
		SERIAL_PORT := $(SERIAL_PORT_CANDIDATES)
	else
		SERIAL_PORT = $(error Black Magic Probe UART serial port not found, please provide the device name via the SERIAL_PORT variable parameter$(if $(SERIAL_PORT_CANDIDATES), (found $(SERIAL_PORT_CANDIDATES))))
	endif
endif

install:
	@echo -n "RAM: "; size "$(PROJECT).elf" | tail -1 | awk '{print $$2 + $$3}'
	@echo -n "ROM: "; size "$(PROJECT).elf" | tail -1 | awk '{print $$1}'

	@printf "INSTALL $(BMP_PORT) $(PROJECT).elf (flash)\n"
	
	$(GDB) -nx --batch \
		-ex 'target extended-remote $(BMP_PORT)' \
		-x bmp.scr \
		$(PROJECT).elf

picocom:
	picocom -b115200 "$(SERIAL_PORT)"
