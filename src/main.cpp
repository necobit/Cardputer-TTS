#include <M5Cardputer.h>
#include <ESP32FormantTTS.h>
#include "i2s_output.h"
#include "romaji_converter.h"

// --- UI レイアウト (240x135, rotation=1) ---
#define SCREEN_W  240
#define SCREEN_H  135
#define TITLE_H   16
#define TEXT_Y    16
#define TEXT_H    80
#define INPUT_Y   96
#define INPUT_H   20
#define STATUS_Y  116
#define STATUS_H  19

static M5Canvas* textCanvas = nullptr;
static RomajiConverter converter;
static bool ttsReady = false;

// ============================================================
//  UI描画
// ============================================================

void drawTitle() {
    M5Cardputer.Display.fillRect(0, 0, SCREEN_W, TITLE_H, TFT_NAVY);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_NAVY);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setFont(&fonts::Font0);
    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.print("Cardputer TTS");
}

void drawText() {
    if (!textCanvas || !textCanvas->getBuffer()) return;
    textCanvas->fillSprite(TFT_BLACK);
    textCanvas->setTextColor(TFT_WHITE);
    textCanvas->setCursor(0, 0);
    textCanvas->print(converter.getHiragana());
    textCanvas->pushSprite(0, TEXT_Y);
}

void drawInput() {
    M5Cardputer.Display.fillRect(0, INPUT_Y, SCREEN_W, INPUT_H, 0x2104);
    M5Cardputer.Display.setTextColor(TFT_YELLOW, 0x2104);
    M5Cardputer.Display.setFont(&fonts::Font0);
    M5Cardputer.Display.setCursor(4, INPUT_Y + 6);
    M5Cardputer.Display.print("> ");
    M5Cardputer.Display.print(converter.getBuffer());
    M5Cardputer.Display.print("_");
}

void drawStatus(const char* msg) {
    M5Cardputer.Display.fillRect(0, STATUS_Y, SCREEN_W, STATUS_H, TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5Cardputer.Display.setFont(&fonts::Font0);
    M5Cardputer.Display.setCursor(4, STATUS_Y + 5);
    M5Cardputer.Display.print(msg);
}

void updateDisplay() {
    drawText();
    drawInput();
}

// ============================================================
//  TTS発声
// ============================================================

void speakText() {
    if (!ttsReady) return;

    String text = converter.getHiragana();
    if (text.length() == 0) return;

    drawStatus("Speaking...");

    char phonemes[MAX_PHONEMES];
    int n = parseHiragana(text.c_str(), phonemes, MAX_PHONEMES);
    if (n > 0) {
        Serial.printf("TTS: %s -> %s (%d)\n", text.c_str(), phonemes, n);
        ttsSynthesize(phonemes);
    } else {
        Serial.println("TTS: No valid hiragana");
    }

    drawStatus("Ready [Enter:speak BS:del]");
}

// ============================================================
//  Setup
// ============================================================

void setup() {
    // M5Cardputer初期化
    M5Cardputer.begin();

    // M5Unifiedのスピーカーを解放（独自I2Sを使うため）
    M5Cardputer.Speaker.end();

    Serial.println("=== Cardputer TTS ===");

    // ディスプレイ設定
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // テキスト表示用Canvas（begin()後に生成）
    textCanvas = new M5Canvas(&M5Cardputer.Display);
    textCanvas->createSprite(SCREEN_W, TEXT_H);
    textCanvas->setFont(&fonts::efontJA_16);
    textCanvas->setTextWrap(true);
    textCanvas->setTextColor(TFT_WHITE);
    Serial.printf("Sprite created. Free heap: %u\n", ESP.getFreeHeap());

    // UI初期描画
    drawTitle();
    drawText();
    drawInput();
    drawStatus("Initializing...");

    // I2S + ES8311 初期化
    if (!initI2S()) {
        drawStatus("I2S Error!");
        Serial.println("ERROR: I2S init failed");
        return;
    }

    // TTS初期化
    ttsInit(writeI2S);
    ttsReady = true;
    Serial.println("TTS ready");

    drawStatus("Ready [Enter:speak BS:del]");

    // デモ発声
    delay(300);
    char phonemes[MAX_PHONEMES];
    int n = parseHiragana("れでぃ", phonemes, MAX_PHONEMES);
    if (n > 0) {
        ttsSynthesize(phonemes);
    }
}

// ============================================================
//  Loop
// ============================================================

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (status.del) {
            // Backspace
            converter.backspace();
            updateDisplay();

        } else if (status.enter) {
            // Enter: バッファ確定 → 発声
            converter.flush();
            updateDisplay();
            speakText();

        } else if (status.word.size() > 0) {
            for (size_t i = 0; i < status.word.size(); i++) {
                char c = status.word[i];
                if (c == ' ') {
                    // Space: バッファ確定（末尾nをんに）
                    converter.flush();
                } else if (c >= 'a' && c <= 'z') {
                    converter.addChar(c);
                } else if (c >= 'A' && c <= 'Z') {
                    converter.addChar(c - 'A' + 'a');
                } else if (c == '-') {
                    // 長音符
                    converter.flush();
                    // 「ー」はUTF-8: 0xE3 0x83 0xBC
                    // parseHiraganaが対応しない場合もあるが表示はする
                }
            }
            updateDisplay();
        }
    }
}
