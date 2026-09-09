#pragma once

#include <LLUtils/StringDefs.h>
#include <OIVImage/OIVBaseImage.h>

namespace IMCodec
{
    class ImageCodec;
}

namespace OIV
{
    class MessageHelper
    {
      public:

        static LLUtils::native_string_type CreateImageInfoMessage(const OIVBaseImageSharedPtr& oivImage,
                                                                  const OIVBaseImageSharedPtr& rasterized,
                                                                  IMCodec::ImageCodec& imageCodec);
        static LLUtils::native_string_type CreateKeyBindingsMessage();
        static LLUtils::native_string_type CreateSystemInfoMessage(const LLUtils::native_string_type& appName,
                                                                    const LLUtils::native_string_type& appVersion,
                                                                    const LLUtils::native_string_type& gitHash,
                                                                    const LLUtils::native_string_type& buildType,
                                                                    const LLUtils::native_string_type& backendName,
                                                                    const LLUtils::native_string_type& gpuName,
                                                                    const LLUtils::native_string_type& apiVersion,
                                                                    const LLUtils::native_string_type& driverVersion,
                                                                    const LLUtils::native_string_type& osName,
                                                                    const LLUtils::native_string_type& cpuCores);
        static LLUtils::native_string_type GetFileTime(const LLUtils::native_string_type& filePath);
    };
}  // namespace OIV
