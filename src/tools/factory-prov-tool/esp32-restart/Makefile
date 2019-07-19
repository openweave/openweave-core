#
#
#   Copyright (c) 2019 Google LLC.
#   All rights reserved.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

#
#   @file
#         Makefile for esp32-restart program.
#

ESP_IDF ?= $(HOME)/tools/esp-idf
TOOL_PREFIX ?=

CFLAGS =                                        \
    -std=gnu99                                  \
    -Os                                         \
    -mtext-section-literals                     \
    -ffunction-sections                         \
    -mlongcalls                                 \
    -nostdlib                                   \
    -fno-builtin                                \
    -MMD                                        \
    -MP                                         \
    -Wl,-static                                 \
    -Wl,--gc-sections

DEFINES =                                       \
    -DESP_PLATFORM                              \
    -DESP32

INCLUDES =                                   	\
    -I$(ESP_IDF)/components/esp32/include       \
    -I$(ESP_IDF)/components/soc/esp32/include   \
    -I$(ESP_IDF)/components/soc/include

PROG_NAME = esp32-restart

all : $(PROG_NAME).hex

$(PROG_NAME) : $(PROG_NAME).c Makefile
	$(TOOL_PREFIX)xtensa-esp32-elf-gcc $(CFLAGS) $(DEFINES) $(INCLUDES) -Wl,-Map=$(PROG_NAME).map -T$(PROG_NAME).ld -o $(PROG_NAME) $(PROG_NAME).c

$(PROG_NAME).bin : $(PROG_NAME)
	$(TOOL_PREFIX)xtensa-esp32-elf-objcopy --dump-section .text=$(PROG_NAME).bin $(PROG_NAME) /dev/null

$(PROG_NAME).hex : $(PROG_NAME).bin
	python3 -c 'import os; any(map(print, ("    " + (" ".join("0x%02X," % b for b in c)) for c in iter(lambda: os.read(0, 16), b""))))' < $(PROG_NAME).bin > $(PROG_NAME).hex
	
clean:
	rm -f $(PROG_NAME) $(PROG_NAME).bin $(PROG_NAME).hex $(PROG_NAME).map $(PROG_NAME).d