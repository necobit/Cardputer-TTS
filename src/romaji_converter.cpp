#include "romaji_converter.h"
#include <string.h>

// ローマ字→ひらがな変換テーブル（最長一致用に長い順でソート）
struct RomajiEntry {
    const char* romaji;
    const char* hiragana;
};

// 3文字エントリ
static const RomajiEntry table3[] = {
    // さ行拡張
    {"sha", "しゃ"}, {"shi", "し"}, {"shu", "しゅ"}, {"sho", "しょ"},
    {"sya", "しゃ"}, {"syu", "しゅ"}, {"syo", "しょ"},
    // た行拡張
    {"chi", "ち"}, {"tsu", "つ"},
    {"cha", "ちゃ"}, {"chu", "ちゅ"}, {"cho", "ちょ"},
    {"tya", "ちゃ"}, {"tyu", "ちゅ"}, {"tyo", "ちょ"},
    // ざ行拡張
    {"zya", "じゃ"}, {"zyu", "じゅ"}, {"zyo", "じょ"},
    // か行拗音
    {"kya", "きゃ"}, {"kyu", "きゅ"}, {"kyo", "きょ"},
    // が行拗音
    {"gya", "ぎゃ"}, {"gyu", "ぎゅ"}, {"gyo", "ぎょ"},
    // な行拗音
    {"nya", "にゃ"}, {"nyu", "にゅ"}, {"nyo", "にょ"},
    // は行拗音
    {"hya", "ひゃ"}, {"hyu", "ひゅ"}, {"hyo", "ひょ"},
    // ま行拗音
    {"mya", "みゃ"}, {"myu", "みゅ"}, {"myo", "みょ"},
    // ら行拗音
    {"rya", "りゃ"}, {"ryu", "りゅ"}, {"ryo", "りょ"},
    // ば行拗音
    {"bya", "びゃ"}, {"byu", "びゅ"}, {"byo", "びょ"},
    // ぱ行拗音
    {"pya", "ぴゃ"}, {"pyu", "ぴゅ"}, {"pyo", "ぴょ"},
    // 小文字（っ）
    {"xtu", "っ"}, {"ltu", "っ"},
    // 小文字（ゃゅょ）
    {"xya", "ゃ"}, {"lya", "ゃ"},
    {"xyu", "ゅ"}, {"lyu", "ゅ"},
    {"xyo", "ょ"}, {"lyo", "ょ"},
};
static const int table3_size = sizeof(table3) / sizeof(table3[0]);

// 2文字エントリ
static const RomajiEntry table2[] = {
    // か行
    {"ka", "か"}, {"ki", "き"}, {"ku", "く"}, {"ke", "け"}, {"ko", "こ"},
    // さ行
    {"sa", "さ"}, {"si", "し"}, {"su", "す"}, {"se", "せ"}, {"so", "そ"},
    // た行
    {"ta", "た"}, {"ti", "ち"}, {"tu", "つ"}, {"te", "て"}, {"to", "と"},
    // な行
    {"na", "な"}, {"ni", "に"}, {"nu", "ぬ"}, {"ne", "ね"}, {"no", "の"},
    // は行
    {"ha", "は"}, {"hi", "ひ"}, {"hu", "ふ"}, {"he", "へ"}, {"ho", "ほ"},
    // ま行
    {"ma", "ま"}, {"mi", "み"}, {"mu", "む"}, {"me", "め"}, {"mo", "も"},
    // や行
    {"ya", "や"}, {"yu", "ゆ"}, {"yo", "よ"},
    // ら行
    {"ra", "ら"}, {"ri", "り"}, {"ru", "る"}, {"re", "れ"}, {"ro", "ろ"},
    // わ行
    {"wa", "わ"}, {"wo", "を"},
    // が行
    {"ga", "が"}, {"gi", "ぎ"}, {"gu", "ぐ"}, {"ge", "げ"}, {"go", "ご"},
    // ざ行
    {"za", "ざ"}, {"zi", "じ"}, {"zu", "ず"}, {"ze", "ぜ"}, {"zo", "ぞ"},
    // だ行
    {"da", "だ"}, {"di", "ぢ"}, {"du", "づ"}, {"de", "で"}, {"do", "ど"},
    // ば行
    {"ba", "ば"}, {"bi", "び"}, {"bu", "ぶ"}, {"be", "べ"}, {"bo", "ぼ"},
    // ぱ行
    {"pa", "ぱ"}, {"pi", "ぴ"}, {"pu", "ぷ"}, {"pe", "ぺ"}, {"po", "ぽ"},
    // ふ（fu）
    {"fu", "ふ"},
    // じゃ行
    {"ja", "じゃ"}, {"ji", "じ"}, {"ju", "じゅ"}, {"jo", "じょ"},
    // ん
    {"nn", "ん"}, {"n'", "ん"},
    // 小文字母音
    {"xa", "ぁ"}, {"xi", "ぃ"}, {"xu", "ぅ"}, {"xe", "ぇ"}, {"xo", "ぉ"},
    {"la", "ぁ"}, {"li", "ぃ"}, {"lu", "ぅ"}, {"le", "ぇ"}, {"lo", "ぉ"},
};
static const int table2_size = sizeof(table2) / sizeof(table2[0]);

// 1文字エントリ（母音のみ）
static const RomajiEntry table1[] = {
    {"a", "あ"}, {"i", "い"}, {"u", "う"}, {"e", "え"}, {"o", "お"},
};
static const int table1_size = sizeof(table1) / sizeof(table1[0]);

RomajiConverter::RomajiConverter() {}

bool RomajiConverter::isVowel(char c) {
    return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o';
}

bool RomajiConverter::isConsonant(char c) {
    return c >= 'a' && c <= 'z' && !isVowel(c);
}

const char* RomajiConverter::lookupTable(const char* key, int len) {
    const RomajiEntry* table;
    int size;

    switch (len) {
        case 3: table = table3; size = table3_size; break;
        case 2: table = table2; size = table2_size; break;
        case 1: table = table1; size = table1_size; break;
        default: return nullptr;
    }

    for (int i = 0; i < size; i++) {
        if (strncmp(key, table[i].romaji, len) == 0 &&
            table[i].romaji[len] == '\0') {
            return table[i].hiragana;
        }
    }
    return nullptr;
}

void RomajiConverter::tryConvert() {
    while (buffer_.length() > 0) {
        bool matched = false;

        // 1. テーブル検索（最長一致: 3→2→1文字）
        int maxLen = buffer_.length() < 3 ? buffer_.length() : 3;
        for (int len = maxLen; len >= 1; len--) {
            const char* result = lookupTable(buffer_.c_str(), len);
            if (result) {
                hiragana_ += result;
                buffer_.remove(0, len);
                matched = true;
                break;
            }
        }
        if (matched) continue;

        // 2. 促音チェック（同じ子音が2つ連続、nnを除く）
        if (buffer_.length() >= 2 &&
            buffer_[0] == buffer_[1] &&
            isConsonant(buffer_[0]) &&
            buffer_[0] != 'n') {
            hiragana_ += "っ";
            buffer_.remove(0, 1); // 2文字目を残す
            continue;
        }

        // 3. 「ん」判定: n + 子音(母音/y/n以外)
        if (buffer_.length() >= 2 &&
            buffer_[0] == 'n' &&
            !isVowel(buffer_[1]) &&
            buffer_[1] != 'y' &&
            buffer_[1] != 'n') {
            hiragana_ += "ん";
            buffer_.remove(0, 1);
            continue;
        }

        // 4. バッファオーバーフロー: 変換不能な先頭文字を出力
        if (buffer_.length() >= 4) {
            // 変換できない文字はそのまま出力
            hiragana_ += buffer_[0];
            buffer_.remove(0, 1);
            continue;
        }

        // 入力待ち
        break;
    }
}

void RomajiConverter::addChar(char c) {
    buffer_ += c;
    tryConvert();
}

void RomajiConverter::flush() {
    // 末尾の 'n' を「ん」に変換
    if (buffer_ == "n") {
        hiragana_ += "ん";
        buffer_ = "";
    }
    // その他の未変換バッファは破棄またはそのまま出力
    if (buffer_.length() > 0) {
        hiragana_ += buffer_;
        buffer_ = "";
    }
}

void RomajiConverter::backspace() {
    if (buffer_.length() > 0) {
        // バッファから1文字削除
        buffer_.remove(buffer_.length() - 1);
    } else if (hiragana_.length() > 0) {
        // ひらがな末尾を削除（UTF-8: ひらがなは3バイト）
        int len = hiragana_.length();
        if (len >= 3) {
            // UTF-8の3バイト文字かチェック
            uint8_t lastByte = hiragana_[len - 1];
            if ((lastByte & 0xC0) == 0x80) {
                // マルチバイト文字の末尾 → 先頭バイトまで遡る
                int pos = len - 1;
                while (pos > 0 && (hiragana_[pos] & 0xC0) == 0x80) {
                    pos--;
                }
                hiragana_.remove(pos);
            } else {
                // ASCII文字
                hiragana_.remove(len - 1);
            }
        } else {
            hiragana_.remove(hiragana_.length() - 1);
        }
    }
}

void RomajiConverter::clear() {
    buffer_ = "";
    hiragana_ = "";
}
