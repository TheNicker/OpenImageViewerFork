#include <ImageLoader.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace
{
    constexpr void WriteUint32(std::array<std::byte, 136>& bytes, std::size_t offset, std::uint32_t value)
    {
        for (std::size_t byteIndex = 0; byteIndex < sizeof(value); ++byteIndex)
            bytes[offset + byteIndex] = static_cast<std::byte>(value >> (byteIndex * 8));
    }

    constexpr auto MakeOnePixelDxt1DDS()
    {
        std::array<std::byte, 136> bytes{};
        bytes[0] = std::byte{'D'};
        bytes[1] = std::byte{'D'};
        bytes[2] = std::byte{'S'};
        bytes[3] = std::byte{' '};

        WriteUint32(bytes, 4, 124);          // DDS_HEADER size
        WriteUint32(bytes, 8, 0x00081007);   // Required fields plus linear size
        WriteUint32(bytes, 12, 1);           // Height
        WriteUint32(bytes, 16, 1);           // Width
        WriteUint32(bytes, 20, 8);           // One DXT1 block
        WriteUint32(bytes, 76, 32);          // DDS_PIXELFORMAT size
        WriteUint32(bytes, 80, 0x4);         // DDPF_FOURCC
        WriteUint32(bytes, 84, 0x31545844);  // DXT1
        WriteUint32(bytes, 108, 0x1000);     // DDSCAPS_TEXTURE

        bytes[128] = std::byte{0x00};  // RGB565 red (0xf800)
        bytes[129] = std::byte{0xf8};
        return bytes;
    }
}  // namespace

TEST_CASE("DDS decoder uses four-byte pixels for partial DXT1 blocks", "[ImageCompatibility][DDS]")
{
    constexpr auto dds = MakeOnePixelDxt1DDS();
    IMCodec::ImageLoader loader;
    IMCodec::ImageSharedPtr image;

    const auto result = loader.Decode(dds.data(), dds.size(), IMCodec::ImageLoadFlags::None, {}, LLUTILS_TEXT("dds"),
                                      IMCodec::PluginTraverseMode::NoTraverse, image);

    REQUIRE(result == IMCodec::ImageResult::Success);
    REQUIRE(image != nullptr);
    REQUIRE(image->GetWidth() == 1);
    REQUIRE(image->GetHeight() == 1);
    REQUIRE(image->GetRowPitchInBytes() == 4);
    REQUIRE(image->GetImageItem()->data.size() == 4);
}
