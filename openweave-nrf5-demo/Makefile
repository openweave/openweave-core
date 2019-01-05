PROJ_DIR := .
OUTPUT_DIRECTORY := build
TEMPLATE_PATH := $(NRF5_SDK_ROOT)/components/toolchain/gcc

TARGET := openweave-nrf5-demo

SRC_FILES := \
    $(PROJ_DIR)/main.cpp \
    $(NRF5_SDK_ROOT)/components/ble/common/ble_advdata.c \
    $(NRF5_SDK_ROOT)/components/ble/common/ble_srv_common.c \
    $(NRF5_SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
    $(NRF5_SDK_ROOT)/components/boards/boards.c \
    $(NRF5_SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
    $(NRF5_SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
    $(NRF5_SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
    $(NRF5_SDK_ROOT)/components/libraries/crc16/crc16.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_aes.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_aes_aead.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_chacha_poly_aead.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_ecc.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_ecdh.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_ecdsa.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_eddsa.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_hash.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_hmac.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_init.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_mutex.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_rng.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310/cc310_backend_shared.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_init.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_rng.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/nrf_hw/nrf_hw_backend_rng_mbedtls.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/nrf_crypto_aes.c \
	$(NRF5_SDK_ROOT)/components/libraries/crypto/nrf_crypto_aes_shared.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/nrf_crypto_init.c \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/nrf_crypto_rng.c \
    $(NRF5_SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
    $(NRF5_SDK_ROOT)/components/libraries/fds/fds.c \
    $(NRF5_SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
    $(NRF5_SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_backend_uart.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
    $(NRF5_SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
    $(NRF5_SDK_ROOT)/components/libraries/mem_manager/mem_manager.c \
    $(NRF5_SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
	$(NRF5_SDK_ROOT)/components/libraries/queue/nrf_queue.c \
    $(NRF5_SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
    $(NRF5_SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
    $(NRF5_SDK_ROOT)/components/libraries/uart/app_uart.c \
    $(NRF5_SDK_ROOT)/components/libraries/uart/app_uart_fifo.c \
    $(NRF5_SDK_ROOT)/components/libraries/uart/retarget.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_error.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_error_weak.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/app_util_platform.c \
    $(NRF5_SDK_ROOT)/components/libraries/util/nrf_assert.c \
    $(NRF5_SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
    $(NRF5_SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
    $(NRF5_SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
    $(NRF5_SDK_ROOT)/external/fprintf/nrf_fprintf.c \
    $(NRF5_SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
    $(NRF5_SDK_ROOT)/external/freertos/portable/CMSIS/nrf52/port_cmsis.c \
    $(NRF5_SDK_ROOT)/external/freertos/portable/CMSIS/nrf52/port_cmsis_systick.c \
    $(NRF5_SDK_ROOT)/external/freertos/portable/GCC/nrf52/port.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/croutine.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/event_groups.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/list.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/portable/MemMang/heap_1.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/queue.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/stream_buffer.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/tasks.c \
    $(NRF5_SDK_ROOT)/external/freertos/source/timers.c \
	$(NRF5_SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
	$(NRF5_SDK_ROOT)/integration/nrfx/legacy/nrf_drv_rng.c \
    $(NRF5_SDK_ROOT)/integration/nrfx/legacy/nrf_drv_uart.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_power_clock.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uart.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uarte.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rng.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
    $(NRF5_SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S \
    $(NRF5_SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c
      
INC_FOLDERS = \
    $(realpath $(PROJ_DIR)) \
    $(NRF5_SDK_ROOT)/components \
    $(NRF5_SDK_ROOT)/components/boards \
    $(NRF5_SDK_ROOT)/components/ble/ble_advertising \
    $(NRF5_SDK_ROOT)/components/ble/common \
    $(NRF5_SDK_ROOT)/components/ble/nrf_ble_gatt \
    $(NRF5_SDK_ROOT)/components/libraries/atomic \
    $(NRF5_SDK_ROOT)/components/libraries/atomic_fifo \
    $(NRF5_SDK_ROOT)/components/libraries/balloc \
    $(NRF5_SDK_ROOT)/components/libraries/bsp \
    $(NRF5_SDK_ROOT)/components/libraries/crc16 \
    $(NRF5_SDK_ROOT)/components/libraries/crypto \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310 \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cc310_bl \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/cifra \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/mbedtls \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/micro_ecc \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/nrf_hw \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/nrf_sw \
    $(NRF5_SDK_ROOT)/components/libraries/crypto/backend/oberon \
    $(NRF5_SDK_ROOT)/components/libraries/delay \
    $(NRF5_SDK_ROOT)/components/libraries/experimental_section_vars \
    $(NRF5_SDK_ROOT)/components/libraries/fds \
    $(NRF5_SDK_ROOT)/components/libraries/fstorage \
    $(NRF5_SDK_ROOT)/components/libraries/log \
    $(NRF5_SDK_ROOT)/components/libraries/log/src \
    $(NRF5_SDK_ROOT)/components/libraries/memobj \
    $(NRF5_SDK_ROOT)/components/libraries/mem_manager \
    $(NRF5_SDK_ROOT)/components/libraries/mutex \
    $(NRF5_SDK_ROOT)/components/libraries/queue \
    $(NRF5_SDK_ROOT)/components/libraries/ringbuf \
    $(NRF5_SDK_ROOT)/components/libraries/stack_info \
    $(NRF5_SDK_ROOT)/components/libraries/strerror \
    $(NRF5_SDK_ROOT)/components/libraries/util \
    $(NRF5_SDK_ROOT)/components/softdevice/common \
    $(NRF5_SDK_ROOT)/components/softdevice/s140/headers \
    $(NRF5_SDK_ROOT)/components/toolchain/cmsis/include \
    $(NRF5_SDK_ROOT)/config/nrf52840/config \
    $(NRF5_SDK_ROOT)/external/fprintf \
    $(NRF5_SDK_ROOT)/external/freertos/config \
    $(NRF5_SDK_ROOT)/external/freertos/portable/CMSIS/nrf52 \
    $(NRF5_SDK_ROOT)/external/freertos/portable/GCC/nrf52 \
    $(NRF5_SDK_ROOT)/external/freertos/source/include \
    $(NRF5_SDK_ROOT)/external/nrf_cc310/include \
    $(NRF5_SDK_ROOT)/integration/nrfx \
    $(NRF5_SDK_ROOT)/integration/nrfx/legacy \
    $(NRF5_SDK_ROOT)/modules/nrfx \
    $(NRF5_SDK_ROOT)/modules/nrfx/drivers/include \
    $(NRF5_SDK_ROOT)/modules/nrfx/hal \
    $(NRF5_SDK_ROOT)/modules/nrfx/mdk \
    $(WEAVE_INCLUDES)

LIB_FILES = \
	$(WEAVE_LIBS) \
	$(NRF5_SDK_ROOT)/external/nrf_cc310/lib/libnrf_cc310_0.9.10.a \
	-lc \
	-lstdc++ \
	-lnosys \
	-lm

LINKER_SCRIPT = $(TARGET).ld

DEBUG ?= 1

ifeq ($(DEBUG),1)
OPT := -g3
else
OPT := -Os -g3
endif

DEFINES = \
    NRF52840_XXAA \
    BOARD_PCA10056 \
    BSP_DEFINES_ONLY \
    CONFIG_GPIO_AS_PINRESET \
    FLOAT_ABI_HARD \
    USE_APP_CONFIG \
    __HEAP_SIZE=0 \
    __STACK_SIZE=8192 \
    SOFTDEVICE_PRESENT
	
CFLAGS = \
    -DBOARD_PCA10056 \
    -DBSP_DEFINES_ONLY \
    -DCONFIG_GPIO_AS_PINRESET \
    -DFLOAT_ABI_HARD \
    -mcpu=cortex-m4 \
    -mthumb \
    -mabi=aapcs \
    -Wall \
    -mfloat-abi=hard \
    -mfpu=fpv4-sp-d16 \
    -ffunction-sections \
    -fdata-sections \
    -fno-strict-aliasing \
    -fshort-enums \
    --specs=nosys.specs \
    $(OPT) \
    $(foreach def,$(DEFINES),-D$(def))

CXXFLAGS = \
    -fno-rtti \
    -fno-exceptions \
    -fno-unwind-tables	

ASMFLAGS := \
    -g3 \
    -mcpu=cortex-m4 \
    -mthumb \
    -mabi=aapcs \
    -mfloat-abi=hard \
    -mfpu=fpv4-sp-d16 \
    $(foreach def,$(DEFINES),-D$(def))

LDFLAGS := \
    -mthumb \
    -mabi=aapcs \
    -L$(NRF5_SDK_ROOT)/modules/nrfx/mdk \
    -mcpu=cortex-m4 \
    -mfloat-abi=hard \
    -mfpu=fpv4-sp-d16 \
    -Wl,--gc-sections \
    --specs=nosys.specs \
    -T$(LINKER_SCRIPT) \
    $(OPT)

all : $(TARGET)

include openweave.mk

include $(TEMPLATE_PATH)/Makefile.common

$(call define_target, $(TARGET))

$(OUTPUT_DIRECTORY)/$(TARGET)/main.cpp.o : install-weave

# Flash the program
flash: $(TARGET)
	@echo Flashing: $(OUTPUT_DIRECTORY)/$(TARGET).hex
	$(NRF5_TOOLS_ROOT)/nrfjprog/nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/$(TARGET).hex --sectorerase
	$(NRF5_TOOLS_ROOT)/nrfjprog/nrfjprog -f nrf52 --reset

flash_softdevice:
	@echo Flashing: s140_nrf52_6.1.0_softdevice.hex
	$(NRF5_TOOLS_ROOT)/nrfjprog/nrfjprog -f nrf52 --program $(NRF5_SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.0_softdevice.hex --sectorerase
	$(NRF5_TOOLS_ROOT)/nrfjprog/nrfjprog -f nrf52 --reset

erase:
	$(NRF5_TOOLS_ROOT)/nrfjprog/nrfjprog -f nrf52 --eraseall

erase:
	$(NRF5_TOOLS_ROOT)/nrfjprog/nrfjprog -f nrf52 --eraseall
