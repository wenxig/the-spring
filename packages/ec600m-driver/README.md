# ec600m ESP-IDF Component

Quectel EC600M (EC600M-CN / EC600M-CNLE) 4G Cat 1 bis cellular driver component for ESP-IDF (target: ESP32-P4).

## Hardware Interface & Pinout (with Waveshare ESP32-P4-WIFI6 + Quectel QTME0068DM EVB)

- **Main AT UART**:
  - `TXD`: Waveshare 40-Pin Header Pin 1 (`GPIO0`) -> 4G EVB J5 Pin 16 (`RX1`)
  - `RXD`: Waveshare 40-Pin Header Pin 2 (`GPIO1`) -> 4G EVB J5 Pin 17 (`TX1`)
  - `GND`: Waveshare 40-Pin Header Pin 3 -> 4G EVB J5 Pin 1/2 (`GND`)
- **Power**:
  - 4G EVB can be powered via standard USB Type-C (5V >= 2A) or 5V pin on J6 Pin 1.
- **Logic Level**:
  - 3.3V TTL (The QTME0068DM EVB includes onboard TXS0108E bidirectional level shifters, compatible with ESP32-P4 3.3V GPIOs).

## Features

- Modern C++23/C++26 & RAII design for ESP-IDF.
- Modular driver wrapping ESP-IDF `esp_modem` with Quectel custom AT commands (`QICSGP`, `QIACT`, `CSQ`, `CEREG`, `QPOWD`).
- Dual-mode support:
  - **Command Mode (AT)**: Signal quality, SIM status, network registration, battery status, positioning.
  - **Data Mode (PPP)**: Seamless integration with `esp_netif` and LwIP TCP/IP stack for standard BSD socket / HTTP / MQTT network communication.
