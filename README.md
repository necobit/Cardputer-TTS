# Cardputer-TTS

M5Stack Cardputer ADV のキーボードからローマ字入力 → ひらがな変換 → 日本語音声合成（TTS）で発声するアプリケーション。

## 動作

1. キーボードからローマ字を入力（リアルタイムでひらがなに変換・表示）
2. Enter キーで発声
3. Backspace で削除、Space で未確定バッファを確定

## ハードウェア

- **M5Stack Cardputer ADV**（ESP32-S3 + ES8311 DAC + TCA8418 キーボード）

## ビルド

[PlatformIO](https://platformio.org/) を使用。

```bash
# ビルド
pio run

# ファームウェア書き込み
pio run -t upload
```

依存ライブラリは `platformio.ini` の `lib_deps` で自動取得されます。

- [M5Cardputer](https://github.com/m5stack/M5Cardputer) - M5Unified ベースの Cardputer ライブラリ
- [ESP32FormantTTS](https://github.com/necobit/ESP32FormantTTS) - フォルマント合成ベースの日本語 TTS

## ローマ字入力

標準的なローマ字入力に対応。

- 母音: `a` `i` `u` `e` `o`
- 拗音: `kya` `sha` `cha` など
- 促音: 子音を重ねる（`kk` → っk）
- 撥音: `nn` または子音の前の `n` が自動変換

## 構成

```
src/
├── main.cpp              # メインループ・UI・キー入力処理
├── i2s_output.cpp/h      # I2S + ES8311 DAC 制御
└── romaji_converter.cpp  # ローマ字→ひらがな変換
include/
└── romaji_converter.h
```

## Cardputer ADV 実装メモ

- **M5Canvas**: `M5Cardputer.begin()` の後に生成すること（static 生成するとディスプレイ未初期化でクラッシュ）
- **I2C**: ES8311 の制御には `m5::In_I2C` を使用。Arduino の `Wire` を使うと TCA8418 キーボードの I2C 通信と競合する
- **スピーカー**: `M5Cardputer.Speaker.end()` で M5Unified のスピーカーを解放してから独自 I2S を初期化

## ライセンス

MIT
