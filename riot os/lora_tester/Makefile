# name of your application
APPLICATION = lora-example

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# other values supported are sx1262, sx1268 and llcc68
LORA_DRIVER ?= sx1262
USEMODULE += $(LORA_DRIVER)
USEMODULE += ztimer_msec

USEMODULE += sx126x_stm32wl

#USEMODULE += stdio_uart
#USEMODULE += stdin
USEMODULE += shell_cmds_default


# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
# DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Use a peripheral timer for the delay, if available
FEATURES_OPTIONAL += periph_timer

include $(RIOTBASE)/Makefile.include

