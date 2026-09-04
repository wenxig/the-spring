# ESP-IDF v5.4/v5.5 Development and Configuration Guide for ESP32-P4-WIFI6

## 1. Environment Setup

### 1.1 Prerequisites & Toolchain
ESP32-P4 requires ESP-IDF **v5.4** or **v5.5** (recommended: ESP-IDF v5.5.x for full RISC-V 360MHz + 32MB PSRAM + PPA 2D hardware support).

```bash
# Clone ESP-IDF v5.5 release branch
git clone -b release/v5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf-v5.5

# Install toolchain for esp32p4 and esp32c6 targets
cd ~/esp/esp-idf-v5.5
./install.sh esp32p4,esp32c6

# Source the environment
. ./export.sh
```

### 1.2 Target Selection
Always set target before building:
```bash
idf.py set-target esp32p4
```

---

## 2. Chip Revision & SDKCONFIG Defaults

Waveshare ESP32-P4 boards ship with either **Chip Revision v3.0 / v3.1** (Standard Production) or early **Pre-v3 (v0.1 / v1.0)** engineering silicon.

In your project `CMakeLists.txt` or `sdkconfig.defaults`:

### Standard Production Chips (Rev v3.0+ / v3.1):
```ini
# Target
CONFIG_IDF_TARGET="esp32p4"
CONFIG_ESP32P4_REV_MIN_3=y

# CPU Clock Configuration
CONFIG_ESP32P4_CPU_CLK_SRC_DEFAULT=y
CONFIG_ESP32P4_CPU_CLK_360M=y

# PSRAM / Octal RAM Configuration (32MB Stacked PSRAM)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_HEX=y
CONFIG_SPIRAM_SPEED_200M=y
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768

# Flash (32MB Quad/Octal SPI)
CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y

# Console Output via USB Serial/JTAG (Type-C)
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y
```

---

## 3. Onboard Peripheral Drivers & Sample Code

### 3.1 Onboard ES8311 I2S Audio Codec + NS4150B PA
The board integrates an ES8311 codec controlled over I2C (`GPIO8` SCL, `GPIO7` SDA) and I2S0 (`GPIO9-GPIO13`), with PA enable on `GPIO53`.

```c
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           7
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000

#define I2S_MCLK_IO                 9
#define I2S_BCLK_IO                 10
#define I2S_WS_IO                   11
#define I2S_DOUT_IO                 12
#define I2S_DIN_IO                  13
#define GPIO_PA_ENABLE              53

static i2s_chan_handle_t tx_chan;
static i2s_chan_handle_t rx_chan;

void audio_init(void) {
    // 1. Enable PA Power
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_PA_ENABLE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_ENABLE, 1); // Turn ON NS4150B PA

    // 2. Initialize I2S Standard Channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_IO,
            .bclk = I2S_BCLK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_DIN_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    i2s_channel_init_std_mode(tx_chan, &std_cfg);
    i2s_channel_init_std_mode(rx_chan, &std_cfg);
    i2s_channel_enable(tx_chan);
    i2s_channel_enable(rx_chan);
}
```

### 3.2 MicroSD / TF Card (4-bit SDMMC Mode)
The board has a MicroSD card slot directly connected to ESP32-P4 SDMMC Slot 0 (GPIO39-44).

```c
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#define MOUNT_POINT "/sdcard"

esp_err_t mount_sdcard(void) {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 40MHz High Speed SDIO

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4; // 4-bit bus mode (GPIO39:D0, 40:D1, 41:D2, 42:D3, 43:CLK, 44:CMD)
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        ESP_LOGI("SD", "SD Card mounted successfully. Capacity: %llu MB",
                 ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
    }
    return ret;
}
```

### 3.3 ESP32-C6 Coprocessor Hosted Communication (esp-hosted-ng)
The onboard ESP32-C6-MINI-1 communicates with the P4 host over SDIO slave interface to provide 802.11ax Wi-Fi 6 and BLE 5.3.

In `sdkconfig`:
```ini
CONFIG_ESP_HOSTED_TRANSPORT_SDIO=y
CONFIG_ESP_WIFI_ENABLED=y
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_50_FEATURES_SUPPORTED=y
```

---

## 4. Flashing & Firmware Tools

### 4.1 USB Serial/JTAG Flashing (Type-C)
Connect the Type-C port to PC. Under Linux / macOS:
```bash
# Build and flash directly
idf.py -p /dev/ttyACM0 flash monitor
```

### 4.2 Recovery / Manual Boot Mode
If the chip is in a reboot loop or firmware is corrupted:
1. Press and hold **BOOT** button.
2. Short press **RST** button.
3. Release **BOOT** button.
4. Execute `idf.py erase-flash` or `esptool.py erase_flash`.
