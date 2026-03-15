#pragma once
#include <ESP32FormantTTS.h>

bool initI2S();
bool writeI2S(const int16_t* buf, size_t frames);
void setES8311Volume(uint8_t vol);
