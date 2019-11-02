PROJECT = fbvclone
BUILD_DIR = bin

SHARED_DIR = ../libfbv ../libpod
CFILES = footctl.c fbv.c pod.c manager.c io.c tick.c stm32.c config.c

DEVICE ?= stm32f030c8
# OOCD_FILE = board/stm32f4discovery.cfg

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
