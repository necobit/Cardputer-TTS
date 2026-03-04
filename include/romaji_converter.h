#pragma once
#include <Arduino.h>

class RomajiConverter {
public:
    RomajiConverter();

    // ローマ字1文字追加して変換を試みる
    void addChar(char c);

    // バッファ内の未確定文字を確定（末尾nをんに変換等）
    void flush();

    // 削除（バッファ中ならバッファから、空なら末尾ひらがな削除）
    void backspace();

    // 結果取得
    const String& getHiragana() const { return hiragana_; }
    const String& getBuffer() const { return buffer_; }

    // 全クリア
    void clear();

private:
    String buffer_;
    String hiragana_;

    void tryConvert();
    static bool isVowel(char c);
    static bool isConsonant(char c);
    static const char* lookupTable(const char* key, int len);
};
