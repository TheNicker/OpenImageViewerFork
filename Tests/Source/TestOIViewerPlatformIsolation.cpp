#include <catch2/catch_all.hpp>

#include <ContextMenu.h>
#include <ImageList.h>
#include <LLUtils/Exception.h>

TEST_CASE("ImageList keeps portable selection state", "[oiviewer][platform]")
{
    ImageList imageList;
    int selectedIndex        = -1;
    auto selectionConnection = imageList.ImageSelectionChanged.Connect(
        [&](const ImageList::ImageSelectionChangeArgs& args) { selectedIndex = args.imageIndex; });

    imageList.SetImage({.index = 0, .title = LLUTILS_TEXT("first")});
    imageList.SetImage({.index = 1, .title = LLUTILS_TEXT("second")});
    imageList.SetSelected(1);

    REQUIRE(imageList.GetNumberOfElements() == 2);
    REQUIRE(imageList.GetSelected() == 1);
    REQUIRE(selectedIndex == 1);
}

TEST_CASE("ContextMenu supports both viewer item models", "[oiviewer][platform]")
{
    struct CommandItem
    {
        std::string command;
        std::string arguments;
    };

    OIV::ContextMenu<int> notificationMenu(0);
    OIV::ContextMenu<CommandItem> commandMenu(0);
    notificationMenu.AddItem(LLUTILS_TEXT("Quit"), 0);
    commandMenu.AddItem(LLUTILS_TEXT("Open"), {.command = "open", .arguments = {}});

    REQUIRE_FALSE(notificationMenu.IsVisible());
    REQUIRE_FALSE(commandMenu.IsVisible());
}

#ifdef OIV_TEST_PLATFORM_LINUX
TEST_CASE("Linux context-menu presentation reports NotImplemented", "[oiviewer][platform]")
{
    LLUtils::Exception::ErrorCode errorCode = LLUtils::Exception::ErrorCode::Unspecified;
    auto exceptionConnection = LLUtils::Exception::OnException.Connect([&](const LLUtils::Exception::EventArgs& args)
                                                                       { errorCode = args.errorCode; });
    OIV::ContextMenu<int> menu(0);

    REQUIRE_THROWS_AS(menu.Show(0, 0, OIV::AlignmentHorizontal::None, OIV::AlignmentVertical::None),
                      LLUtils::Exception);
    REQUIRE(errorCode == LLUtils::Exception::ErrorCode::NotImplemented);
}
#endif
