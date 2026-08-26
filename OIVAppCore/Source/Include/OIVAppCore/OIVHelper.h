#pragma once

#include <Image.h>
#include <LLUtils/Point.h>

#include <string>

namespace OIV
{
    class OIVHelper
    {
      public:

        static const std::string& PickColor(IMCodec::ChannelSemantic semantic);
        static std::string ParseTexelValue(IMCodec::ImageSharedPtr image, LLUtils::PointI32 pixelPos);
    };
}  // namespace OIV
