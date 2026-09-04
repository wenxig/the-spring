# Waveshare ESP32-P4-WIFI6 Hardware Architecture & 40-Pin Header Pinout

## 1. System Overview & Onboard Hardware

The Waveshare ESP32-P4-WIFI6 development board couples the high-performance ESP32-P4 RISC-V SoC with an ESP32-C6 wireless companion module.

### Core Processing & Memory
- **SoC**: Espressif ESP32-P4NRW32 (QFN-132)
  - **HP CPU**: Dual-core RISC-V 32-bit (IMC instruction set), running up to 360 MHz (Revision v3.0+) or 400 MHz (experimental).
  - **LP CPU**: Ultra-low-power RISC-V core @ 40 MHz.
  - **Internal SRAM**: 768 KB L2 memory (512 KB HP SRAM + 128 KB dedicated HP SRAM cache + 32 KB LP memory).
  - **Stacked PSRAM**: 32 MB 200 MHz Octal DTR PSRAM (integrated inside P4NRW32 package).
  - **External Flash**: 32 MB (256 Mbit) Quad SPI NOR Flash.

### Wireless Coprocessor
- **Module**: ESP32-C6-MINI-1 (single-core 32-bit RISC-V @ 160 MHz)
- **Protocol**: 2.4 GHz Wi-Fi 6 (802.11ax/b/g/n, 20 MHz / 40 MHz bandwidth), Bluetooth 5 (LE), Zigbee / Thread (802.15.4).
- **Interconnect Bus**: Connected to ESP32-P4 via SDIO slave mode.
- **Role**: Offloads Wi-Fi TCP/IP and Bluetooth stacks from ESP32-P4, communicating via ESP-Hosted / SDIO driver.

### Multimedia & Display Interfaces
- **MIPI-DSI**: 2-lane High Speed D-PHY (up to 1.5 Gbps/lane), supports display resolutions up to 1080p @ 60 Hz or 720p with capacitive touch. Standard 15-pin 1.0mm FPC connector.
- **MIPI-CSI**: 2-lane High Speed D-PHY with built-in hardware ISP (Auto Exposure, Auto White Balance, Lens Correction, Demosaicing) and H.264 video hardware encoder (up to 1080p @ 30 fps). Standard 15-pin 1.0mm FPC connector.
- **2D Graphics Accelerator (PPA)**: Pixel Processing Accelerator hardware engine supporting scaling, color format conversion (RGB565, RGB888, YUV), alpha blending, and rotation.

---

## 2. Onboard Dedicated Peripheral Pin Assignments

The following GPIOs of ESP32-P4 are dedicated to onboard peripherals and **must not** be driven or redefined by external circuits:

| Function | Pin / GPIO | Description / Signal Name |
| :--- | :--- | :--- |
| **Audio Codec (ES8311)** | GPIO9 | `I2S_MCLK` (Master Clock) |
| | GPIO10 | `I2S_BCLK` (Bit Clock) |
| | GPIO11 | `I2S_WS` / `LRCK` (Word Select / Left-Right Clock) |
| | GPIO12 | `I2S_SDIN` / `DOUT` (Audio data from ES8311 to P4 / MIC) |
| | GPIO13 | `I2S_SDOUT` / `DIN` (Audio data from P4 to ES8311 / Speaker) |
| | GPIO53 | `PA_EN` (NS4150B Class-D power amplifier enable, active High) |
| **I2C Bus (Audio & Touch & Cam)**| GPIO8 | `I2C0_SCL` (Clock for ES8311 & Camera SCCB & Touch GT911) |
| | GPIO7 | `I2C0_SDA` (Data for ES8311 & Camera SCCB & Touch GT911) |
| **MicroSD / TF Card (SDMMC)** | GPIO43 | `SDMMC_CLK` (Clock) |
| | GPIO44 | `SDMMC_CMD` (Command / Response) |
| | GPIO39 | `SDMMC_D0` (Data bit 0) |
| | GPIO40 | `SDMMC_D1` (Data bit 1) |
| | GPIO41 | `SDMMC_D2` (Data bit 2) |
| | GPIO42 | `SDMMC_D3` (Data bit 3 / Card Detect pull-up) |
| **USB 2.0 High-Speed OTG** | GPIO27 | `USB_DP` (480 Mbps High Speed Data +) |
| | GPIO26 | `USB_DM` (480 Mbps High Speed Data -) |
| **USB-UART / JTAG Type-C** | Native USB | Native USB Serial/JTAG debugging port |
| **Wireless ESP32-C6 Link** | SDIO Lines | High-speed SDIO bus for ESP-Hosted network offload |
| **Buttons & Status** | GPIO35 (or BOOT) | BOOT Button (Active LOW for ROM download mode) |
| | CHIP_PU / RST | Hardware Reset Button (Active LOW) |

---

## 3. 40-Pin Expansion Header (Raspberry Pi Pico HAT Form Factor)

The board features a standard 2×20 2.54mm dual-row header mechanically and electrically aligned with the Raspberry Pi Pico expansion standard.

### Pinout Mapping Table

```
           +------------------+
    GPIO0  |  1            40 |  VBUS / VSYS (5V Input / Output)
    GPIO1  |  2            39 |  VSYS (5V In)
      GND  |  3            38 |  GND
    GPIO2  |  4            37 |  3V3_EN (Power Supply Enable)
    GPIO3  |  5            36 |  3V3_OUT (3.3V System Output)
    GPIO4  |  6            35 |  ADC_VREF
    GPIO5  |  7            34 |  GPIO25 (ADC0_CH2)
      GND  |  8            33 |  GND
    GPIO6  |  9            32 |  GPIO24 (ADC0_CH1)
   (GPIO7) | 10            31 |  GPIO23 (ADC0_CH0)
   (GPIO8) | 11            30 |  RUN / RST (Reset)
    GPIO9  | 12            29 |  GPIO22
      GND  | 13            28 |  GND
   GPIO10  | 14            27 |  GPIO21
   GPIO11  | 15            26 |  GPIO20
   GPIO12  | 16            25 |  GPIO19
   GPIO13  | 17            24 |  GPIO18
      GND  | 18            23 |  GND
   GPIO14  | 19            22 |  GPIO17
   GPIO15  | 20            21 |  GPIO16
           +------------------+
```

*Note: GPIO7, 8, 9, 10, 11, 12, 13 are shared with the onboard I2C/I2S buses. For clean external peripheral use on a carrier board, prioritize the uncommitted pins listed below.*

### Available Uncommitted Pins for Carrier Board Design
- **Free Digital GPIOs**: `GPIO0`, `GPIO1`, `GPIO2`, `GPIO3`, `GPIO4`, `GPIO5`, `GPIO6`, `GPIO14`, `GPIO15`, `GPIO16`, `GPIO17`, `GPIO18`, `GPIO19`, `GPIO20`, `GPIO21`, `GPIO22`, `GPIO32`.
- **Analog Inputs**: `GPIO23` (ADC0_CH0), `GPIO24` (ADC0_CH1), `GPIO25` (ADC0_CH2).
- **Power Rails**:
  - `VSYS / 5V` (Pins 39 & 40): Main 5V power supply from USB Type-C or external 5V regulator.
  - `3V3_OUT` (Pin 36): Regulated 3.3V output from onboard LDO/DC-DC (drives external 3.3V logic).
  - `GND` (Pins 3, 8, 13, 18, 23, 28, 33, 38): Clean return ground planes.

---

## 4. Carrier Board Pin Allocation for EC600M + Landline (RJ11 FXS) + E-Ink

When designing the custom baseboard combining the Waveshare ESP32-P4-WIFI6 with Quectel EC600M, RJ11 FXS SLIC, 6 Navigation Buttons, and E-Ink display, map the uncommitted 40-pin header pins as follows:

| Target Function | Peripheral Signal | Recommended P4 Header Pin | GPIO | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Quectel EC600M** | UART_TXD (MCU -> EC600M) | Pin 1 | GPIO0 | Goes to TXS0108E 3.3V side B1 |
| | UART_RXD (EC600M -> MCU) | Pin 2 | GPIO1 | From TXS0108E 3.3V side B2 |
| | UART_RTS (MCU -> EC600M) | Pin 4 | GPIO2 | Hardware flow control RTS |
| | UART_CTS (EC600M -> MCU) | Pin 5 | GPIO3 | Hardware flow control CTS |
| | 4G_DTR (Sleep control) | Pin 6 | GPIO4 | Wake-up / Sleep toggle via TXS0108E |
| | 4G_RI (Ring / URC interrupt)| Pin 7 | GPIO5 | Ring indicator interrupt input |
| | 4G_STATUS (Module Power Ind)| Pin 9 | GPIO6 | High when EC600M is booted |
| | 4G_PWRKEY (Power on/off) | Pin 19 | GPIO14 | Drives NPN (MMBT3904) base |
| | 4G_RESET_N (Hardware Reset)| Pin 20 | GPIO15 | Drives NPN (MMBT3904) base |
| **E-Ink Display (SPI)** | EINK_SCLK (SPI Clock) | Pin 21 | GPIO16 | High-speed SPI clock |
| | EINK_MOSI (SPI Master Out) | Pin 22 | GPIO17 | Display SPI command/data |
| | EINK_CS (Chip Select) | Pin 24 | GPIO18 | Active LOW chip select |
| | EINK_DC (Data/Command) | Pin 25 | GPIO19 | High = Data, Low = Command |
| | EINK_RST (Hardware Reset) | Pin 26 | GPIO20 | Reset pin for display driver |
| | EINK_BUSY (Status Busy) | Pin 27 | GPIO21 | Input (High = Busy refreshing) |
| **RJ11 Landline (FXS)** | SLIC_SCLK (SPI Clock) | Pin 21 (Shared) | GPIO16 | Shared SPI Bus with E-Ink |
| *(Si32178 / Si32176)* | SLIC_MOSI (SPI Master Out)| Pin 22 (Shared) | GPIO17 | Shared SPI Bus with E-Ink |
| | SLIC_MISO (SPI Master In) | Pin 29 | GPIO22 | SPI read data from SLIC |
| | SLIC_CS (Chip Select) | Pin 31 | GPIO23 | Dedicated SLIC chip select |
| | SLIC_INT (Interrupt) | Pin 32 | GPIO24 | Hook switch state / Ring detect |
| | SLIC_RESET_N | Pin 34 | GPIO25 | SLIC hardware reset |
| **6 Physical Keys** | KEY_UP | Matrix or ADC | - | Keypad matrix or dedicated GPIOs |
| | KEY_DOWN | Matrix or ADC | - | Can also be routed via I2C expander |
| | KEY_LEFT | Matrix or ADC | - | or multi-button resistor ladder |
| | KEY_RIGHT | Matrix or ADC | - | into GPIO25 (ADC0_CH2) |
| | KEY_CONFIRM / OK | Matrix or ADC | - | |
| | KEY_BACK / CANCEL | Matrix or ADC | - | |
