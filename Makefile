DEVELHELP ?= 0
include ../Makefile.drivers_common
include ../../periph/Makefile.periph_common
include ../../sys/Makefile.sys_common

DRIVER ?= lps331ap

USEMODULE += $(DRIVER)
USEMODULE += ztimer ztimer_usec ztimer_msec ztimer_sec
USEPKG += semtech-loramac
USEMODULE += $(LORA_DRIVER)
USEMODULE += fmt
USEMODULE += auto_init_loramac
LORA_DRIVER ?= sx1272
LORA_REGION ?= EU868

FEATURES_OPTIONAL += periph_eeprom
FEATURES_OPTIONAL += periph_rtc
FEATURES_REQUIRED += periph_wdt

include $(RIOTBASE)/Makefile.include
