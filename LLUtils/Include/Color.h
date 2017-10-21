#pragma once
#include <algorithm>
namespace LLUtils
{
    struct Color
    {
        union
        {
            struct
            {
                uint8_t R, G, B, A;
            };
            uint32_t colorValue;
        };

        Color() {}
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            R = r;
            B = b;
            G = g;
            A = a;
        }
        Color(uint32_t color)
        {
            colorValue = color;
        }

        Color Blend(const Color& source)
        {
            Color blended;
            uint8_t invSourceAlpha = 0xFF - source.A;
            blended.R = (source.A * source.R + invSourceAlpha * R) / 0xFF;
            blended.G = (source.A * source.G + invSourceAlpha * G) / 0xFF;
            blended.B = (source.A * source.B + invSourceAlpha * B) / 0xFF;
            blended.A = std::max(A, source.A);
            return blended;
        }
        static Color FromString(const std::string& str)
        {
            using namespace std;;
            Color c = 0xFF << 24;
            char strByteColor[3];
            strByteColor[2] = 0;
            if (str.length() > 0)
            {
                if (str[0] == '#')
                {
                    string hexValues = str.substr(1);
                    if (hexValues.length() > 8)
                        hexValues.erase(hexValues.length());

                    int length = hexValues.length();
                    int i = 0;
                    while (i < length)
                    {
                        if (length - i >= 2)
                        {
                            strByteColor[0] = hexValues[i];
                            strByteColor[1] = hexValues[i + 1];
                            uint8_t val = std::strtol(strByteColor, nullptr, 16);
                            *(reinterpret_cast<uint8_t*>(&c.colorValue) + i / 2) = val;
                            i += 2;
                        }
                        else if (length - i == 1)
                        {
                            strByteColor[0] = '0';
                            strByteColor[1] = hexValues[i];
                            uint8_t val = std::strtol(strByteColor, nullptr, 16);
                            *(reinterpret_cast<uint8_t*>(&c.colorValue) + i / 2) = val;
                            i += 2;
                        }

                    }

                }

            }
            return c;
        }
    };
}