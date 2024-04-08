/**
 * VGS-Zero - SDK for Steam
 * License under GPLv3: https://github.com/suzukiplan/vgszero/blob/master/LICENSE-VGS0.txt
 * (C)2024, SUZUKI PLAN
 */
#pragma once

class KeyConfig
{
  public:
    std::string pad;
    unsigned char button;

    KeyConfig(const char* pad, unsigned char button)
    {
        this->pad = pad;
        this->button = button;
    }
};
