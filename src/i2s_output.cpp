#include "i2s_output.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <driver/i2s_std.h>

// --- I2S ピン (StampS3 + ES8311 = Cardputer ADV) ---
#define I2S_BCK_PIN  GPIO_NUM_41
#define I2S_WS_PIN   GPIO_NUM_43
#define I2S_DOUT_PIN GPIO_NUM_42

// --- ES8311 I2Cアドレス ---
#define ES8311_ADDR  0x18
#define ES8311_I2C_FREQ 100000

static i2s_chan_handle_t tx_handle = NULL;

// ============================================================
//  ES8311 コーデック制御（M5Unified In_I2C使用）
// ============================================================

static bool es8311WriteReg(uint8_t reg, uint8_t val) {
    return m5::In_I2C.writeRegister8(ES8311_ADDR, reg, val, ES8311_I2C_FREQ);
}

static uint8_t es8311ReadReg(uint8_t reg) {
    return m5::In_I2C.readRegister8(ES8311_ADDR, reg, ES8311_I2C_FREQ);
}

static bool initES8311() {
    delay(10);

    // ES8311の存在確認
    uint8_t id1 = es8311ReadReg(0xFD);
    uint8_t id2 = es8311ReadReg(0xFE);
    uint8_t ver = es8311ReadReg(0xFF);
    if (id1 == 0xFF && id2 == 0xFF) {
        Serial.println("ERROR: ES8311 not found at I2C 0x18");
        return false;
    }
    Serial.printf("ES8311 found: ID=0x%02X%02X ver=0x%02X\n", id1, id2, ver);

    // ソフトリセット
    es8311WriteReg(0x00, 0x1F);
    delay(20);
    es8311WriteReg(0x00, 0x00);
    es8311WriteReg(0x00, 0x80);

    // クロック設定 (MCLKはSCLKピンから)
    es8311WriteReg(0x01, 0xBF);
    es8311WriteReg(0x02, 0x18);  // pre_div=1, pre_multi=8x
    es8311WriteReg(0x03, 0x10);
    es8311WriteReg(0x04, 0x10);
    es8311WriteReg(0x05, 0x00);
    es8311WriteReg(0x06, 0x03);  // bclk_div=4
    es8311WriteReg(0x07, 0x00);
    es8311WriteReg(0x08, 0xFF);  // LRCK divider = 256

    // SDPフォーマット: I2S, 16bit
    es8311WriteReg(0x09, 0x0C);
    es8311WriteReg(0x0A, 0x0C);

    // アナログ部パワーアップ
    es8311WriteReg(0x0D, 0x01);
    es8311WriteReg(0x0E, 0x02);

    // DAC有効化
    es8311WriteReg(0x12, 0x00);
    es8311WriteReg(0x13, 0x10);

    // EQバイパス
    es8311WriteReg(0x1C, 0x6A);
    es8311WriteReg(0x37, 0x08);

    // ボリューム（最大）
    es8311WriteReg(0x32, 0xFF);

    delay(50);
    Serial.println("ES8311 DAC initialized (32kHz)");
    return true;
}

// ============================================================
//  I2S ドライバ
// ============================================================

bool initI2S() {
    if (!initES8311()) {
        Serial.println("WARNING: ES8311 init failed");
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 256;

    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_new_channel: %s\n", esp_err_to_name(err));
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(TTS_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_channel_init: %s\n", esp_err_to_name(err));
        return false;
    }

    err = i2s_channel_enable(tx_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: i2s_channel_enable: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println("I2S initialized (32kHz, 16bit, stereo)");
    return true;
}

bool writeI2S(const int16_t* buf, size_t frames) {
    size_t bytes_written = 0;
    size_t byte_size = frames * 2 * sizeof(int16_t);
    esp_err_t err = i2s_channel_write(tx_handle, buf, byte_size,
                                       &bytes_written, portMAX_DELAY);
    return err == ESP_OK;
}
