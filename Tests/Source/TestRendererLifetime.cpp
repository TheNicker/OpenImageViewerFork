#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ViewerRenderPort.h"
#include "ApiGlobal.h"
#include "OIV.h"
#include <OIVImage/OIVBaseImage.h>

namespace
{
    // Each test owns its API instance without disturbing the suite's other image tests.
    struct ScopedApi
    {
        std::unique_ptr<OIV::IPictureRenderer> previous = std::move(OIV::ApiGlobal::sPictureRenderer);

        ScopedApi() { OIV::ApiGlobal::sPictureRenderer = std::make_unique<OIV::OIV>(); }
        ~ScopedApi() { OIV::ApiGlobal::sPictureRenderer = std::move(previous); }
    };
}  // namespace

TEST_CASE("Render gateway releases the API after images on success or initialization failure", "[renderer][lifetime]")
{
    ScopedApi api;
    const bool fail = GENERATE(false, true);
    const OIV::RendererOptions options{.renderer = fail ? static_cast<OIV::RendererType>(-1) : OIV::RendererType::Null};
    {
        OIV::OivRenderGateway gateway;
        OIV::OIVBaseImage image(OIV::ImageSource::GeneratedByLib);
        if (fail)
            REQUIRE_THROWS_WITH(gateway.Initialize(0, nullptr, options),
                                Catch::Matchers::ContainsSubstring("not available"));
        else
        {
            REQUIRE_NOTHROW(gateway.Initialize(0, nullptr, options));
            REQUIRE(std::string(OIV::ApiGlobal::sPictureRenderer->GetRenderer()->GetBackendName()) == "Null");
        }
        // The image's destructor needs the API, so it runs before the gateway releases it.
        REQUIRE(OIV::ApiGlobal::sPictureRenderer != nullptr);
    }
    CHECK(OIV::ApiGlobal::sPictureRenderer == nullptr);
}

TEST_CASE("Unused render gateway leaves the API available", "[renderer][lifetime]")
{
    ScopedApi api;
    {
        OIV::OivRenderGateway gateway;
    }
    CHECK(OIV::ApiGlobal::sPictureRenderer != nullptr);
}

TEST_CASE("Renderer shutdown disconnects its global exception subscription", "[renderer][lifetime]")
{
    ScopedApi api;
    const OIV::RendererOptions options{.renderer = OIV::RendererType::Null};
    int callbacks = 0;
    {
        OIV::OivRenderGateway gateway;
        REQUIRE_NOTHROW(gateway.Initialize(0, nullptr, options));
        const OIV_CMD_RegisterCallbacks_Request callback{.OnException = [](OIV_Exception_Args, void* userPointer)
                                                         { ++*static_cast<int*>(userPointer); },
                                                         .userPointer = &callbacks};
        REQUIRE(OIV::ApiGlobal::sPictureRenderer->RegisterCallbacks(callback) == RC_Success);
        LLUtils::Exception::OnException.Raise(LLUtils::Exception::EventArgs{});
        REQUIRE(callbacks == 1);
    }
    LLUtils::Exception::OnException.Raise(LLUtils::Exception::EventArgs{});
    CHECK(callbacks == 1);
}

TEST_CASE("Render gateway forwards zero extents and restores the same viewport", "[renderer][lifetime]")
{
    ScopedApi api;
    const OIV::RendererOptions options{.renderer = OIV::RendererType::Null};
    OIV::OivRenderGateway gateway;
    gateway.Initialize(0, nullptr, options);
    const auto& renderer = static_cast<const OIV::OIV&>(*OIV::ApiGlobal::sPictureRenderer);

    for (const auto size : {LWS::PixelSize{}, LWS::PixelSize{640, 480}, LWS::PixelSize{}, LWS::PixelSize{640, 480},
                            LWS::PixelSize{640, 480}})
    {
        REQUIRE(gateway.SetViewportSize(size) == RC_Success);
        REQUIRE(renderer.GetClientSize() == LLUtils::PointI32{size.x, size.y});
    }
}
