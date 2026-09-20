#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "sd_test";

// Waveshare ESP32-P4-DEV-KIT SD 卡引脚
#define SD_CLK  GPIO_NUM_43
#define SD_CMD  GPIO_NUM_44
#define SD_D0   GPIO_NUM_39
#define SD_D1   GPIO_NUM_40
#define SD_D2   GPIO_NUM_41
#define SD_D3   GPIO_NUM_42

void test_sd_card(void) {
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "SD Card Hardware Test");
    ESP_LOGI(TAG, "========================================\n");
    
    ESP_LOGI(TAG, "SD Card Pin Configuration:");
    ESP_LOGI(TAG, "  CLK  = GPIO%d", SD_CLK);
    ESP_LOGI(TAG, "  CMD  = GPIO%d", SD_CMD);
    ESP_LOGI(TAG, "  D0   = GPIO%d", SD_D0);
    ESP_LOGI(TAG, "  D1   = GPIO%d", SD_D1);
    ESP_LOGI(TAG, "  D2   = GPIO%d", SD_D2);
    ESP_LOGI(TAG, "  D3   = GPIO%d", SD_D3);
    ESP_LOGI(TAG, "");
    
    // 测试配置 1: 1-bit 模式 + 400kHz（最兼容）
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Test 1: 1-bit mode, 400kHz (PROBING)");
    ESP_LOGI(TAG, "========================================");
    
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_PROBING;  // 400kHz
    
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;  // 1-bit
    slot.clk = SD_CLK;
    slot.cmd = SD_CMD;
    slot.d0 = SD_D0;
    slot.d1 = SD_D1;
    slot.d2 = SD_D2;
    slot.d3 = SD_D3;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false,
    };
    
    sdmmc_card_t *card = NULL;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mount_config, &card);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ SD card mounted successfully!");
        ESP_LOGI(TAG, "Card info:");
        ESP_LOGI(TAG, "  Name: %s", card->cid.name);
        ESP_LOGI(TAG, "  Type: %s", (card->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC");
        ESP_LOGI(TAG, "  Speed: %s", (card->csd.tr_speed > 25000000) ? "high speed" : "default speed");
        ESP_LOGI(TAG, "  Size: %llu MB", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
        ESP_LOGI(TAG, "  CSD: ver=%d, sector_size=%d, capacity=%d read_bl_len=%d",
                 card->csd.csd_ver, card->csd.sector_size, card->csd.capacity, card->csd.read_block_len);
        ESP_LOGI(TAG, "  SCR: sd_spec=%d, bus_width=%d", card->scr.sd_spec, card->scr.bus_width);
        
        // 尝试写入测试文件
        ESP_LOGI(TAG, "\nTesting file write...");
        FILE *f = fopen("/sdcard/test.txt", "w");
        if (f != NULL) {
            fprintf(f, "ESP32-P4 SD Card Test\n");
            fprintf(f, "Hardware: Waveshare ESP32-P4-Module-DEV-KIT\n");
            fprintf(f, "Timestamp: %lld\n", (long long)esp_timer_get_time());
            fclose(f);
            ESP_LOGI(TAG, "✅ File write successful");
            
            // 读取测试
            f = fopen("/sdcard/test.txt", "r");
            if (f != NULL) {
                char line[128];
                ESP_LOGI(TAG, "\nFile content:");
                while (fgets(line, sizeof(line), f)) {
                    ESP_LOGI(TAG, "  %s", line);
                }
                fclose(f);
                ESP_LOGI(TAG, "✅ File read successful");
            } else {
                ESP_LOGE(TAG, "❌ Failed to read file");
            }
        } else {
            ESP_LOGE(TAG, "❌ Failed to write file");
        }
        
        // 卸载
        esp_vfs_fat_sdcard_unmount("/sdcard", card);
        ESP_LOGI(TAG, "SD card unmounted");
        
    } else {
        ESP_LOGE(TAG, "❌ SD card mount failed: %s (0x%x)", esp_err_to_name(ret), ret);
        
        if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "Possible causes:");
            ESP_LOGE(TAG, "  1. SD card not inserted");
            ESP_LOGE(TAG, "  2. SD card not fully inserted (check the click)");
            ESP_LOGE(TAG, "  3. SD card damaged or incompatible");
            ESP_LOGE(TAG, "  4. SD card slot hardware fault");
            ESP_LOGE(TAG, "  5. SD card formatted incorrectly (must be MBR + FAT32)");
            ESP_LOGE(TAG, "");
            ESP_LOGE(TAG, "Troubleshooting steps:");
            ESP_LOGE(TAG, "  1. Remove and re-insert the SD card firmly");
            ESP_LOGE(TAG, "  2. Try a different SD card (use <= 32GB, FAT32)");
            ESP_LOGE(TAG, "  3. Format on computer as MBR + FAT32");
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 测试配置 2: 4-bit 模式 + 20MHz（标准速度）
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "Test 2: 4-bit mode, 20MHz (DEFAULT)");
    ESP_LOGI(TAG, "========================================");
    
    sdmmc_host_t host2 = SDMMC_HOST_DEFAULT();
    host2.max_freq_khz = SDMMC_FREQ_DEFAULT;  // 20MHz
    
    sdmmc_slot_config_t slot2 = SDMMC_SLOT_CONFIG_DEFAULT();
    slot2.width = 4;  // 4-bit
    slot2.clk = SD_CLK;
    slot2.cmd = SD_CMD;
    slot2.d0 = SD_D0;
    slot2.d1 = SD_D1;
    slot2.d2 = SD_D2;
    slot2.d3 = SD_D3;
    slot2.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    
    card = NULL;
    ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host2, &slot2, &mount_config, &card);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ SD card mounted successfully (4-bit mode)!");
        ESP_LOGI(TAG, "Card info:");
        ESP_LOGI(TAG, "  Name: %s", card->cid.name);
        ESP_LOGI(TAG, "  Size: %llu MB", ((uint64_t)card->csd.capacity) * card->csd.sector_size / (1024 * 1024));
        
        esp_vfs_fat_sdcard_unmount("/sdcard", card);
        ESP_LOGI(TAG, "SD card unmounted");
    } else {
        ESP_LOGE(TAG, "❌ SD card mount failed (4-bit): %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "\n========================================");
    ESP_LOGI(TAG, "SD Card Test Complete");
    ESP_LOGI(TAG, "========================================");
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32-P4 SD Card Hardware Test");
    ESP_LOGI(TAG, "Firmware: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    test_sd_card();
    
    ESP_LOGI(TAG, "\nTest finished. System will idle.");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
