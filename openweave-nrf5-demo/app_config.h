#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define MEM_MANAGER_ENABLED 1
#define MEMORY_MANAGER_SMALL_BLOCK_COUNT 4
#define MEMORY_MANAGER_SMALL_BLOCK_SIZE 32
#define MEMORY_MANAGER_MEDIUM_BLOCK_COUNT 4
#define MEMORY_MANAGER_MEDIUM_BLOCK_SIZE 256
#define MEMORY_MANAGER_LARGE_BLOCK_COUNT 1
#define MEMORY_MANAGER_LARGE_BLOCK_SIZE 1024

#define NRF_FPRINTF_ENABLED 1

#define NRF_STRERROR_ENABLED 1

#define NRF_CRYPTO_ENABLED 1
#define NRF_CRYPTO_BACKEND_CC310_ENABLED 1
#define NRF_CRYPTO_RNG_AUTO_INIT_ENABLED 0
#define NRF_CRYPTO_BACKEND_NRF_HW_RNG_ENABLED 0
#define NRF_CRYPTO_BACKEND_CC310_RNG_ENABLED 1
#define NRF_CRYPTO_AES_ENABLED 1
#define NRF_CRYPTO_CC310_AES_ENABLED 1
#define NRF_CRYPTO_BACKEND_CC310_AES_ECB_ENABLED 1
#define NRF_CRYPTO_BACKEND_NRF_HW_RNG_MBEDTLS_CTR_DRBG_ENABLED 0
#define NRF_CRYPTO_RNG_STATIC_MEMORY_BUFFERS_ENABLED 1
#define NRFX_RNG_CONFIG_ERROR_CORRECTION 1
#define RNG_ENABLED 1
#define RNG_CONFIG_ERROR_CORRECTION 1

#define NRF_QUEUE_ENABLED 1

#define SOFTDEVICE_PRESENT 1
#define NRF_SDH_ENABLED 1
#define NRF_SDH_SOC_ENABLED 1

#define FDS_ENABLED 1
#define FDS_BACKEND NRF_FSTORAGE_SD

#define NRF_FSTORAGE_ENABLED 1

#define NRF_CLOCK_ENABLED 1

#define NRF_LOG_ENABLED 1
#define NRF_LOG_BACKEND_UART_ENABLED 1
#define NRF_LOG_BACKEND_UART_TX_PIN 6
#define NRF_LOG_BACKEND_UART_BAUDRATE 30801920
#define NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE 64
#define NRF_LOG_STR_PUSH_BUFFER_SIZE 4096

#define UART_ENABLED 1
#define UART0_ENABLED 1
#define NRFX_UARTE_ENABLED 1
#define NRFX_UART_ENABLED 1
#define UART_LEGACY_SUPPORT 0

#endif //APP_CONFIG_H

